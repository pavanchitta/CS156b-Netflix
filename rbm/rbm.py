import tensorflow as tf
import numpy as np

from tensorflow import contrib
tfe = tf.contrib.eager

class RBM:
    def __init__(self):
        self.n_visible = 17770
        self.batch_size = 128
        self.n_hidden = 100
        self.momentum = tf.constant(0.9)
        self.weight_decay = tf.constant(0.0)
        self.k = 1
        self.num_rat = 5
        self.lr_weights = tf.constant(0.0015)
        self.lr_vb = tf.constant(0.0015)
        self.lr_hb = tf.constant(0.1)

        self.anneal = False
        self.anneal_val = tf.constant(0.0)
        self.num_users = 458293

        # if momentum:
        #     self.momentum = tf.placeholder(tf.float32)
        # else:
        #     self.momentum = 0.0
        # Initialize weights and biases
        initial_weights = tf.random.normal([self.n_visible, self.num_rat, self.n_hidden], stddev=0.01)
        # initial_weights = tf.reduce_mean(initial_weights, axis=1)
        # initial_weights = tf.stack([initial_weights, initial_weights, initial_weights, initial_weights, initial_weights], axis=1)
        self.weights = tfe.Variable(initial_weights, name="weights")


        arr = np.loadtxt("movie_frequencies.dta")

        visible_bias = tf.to_float(tf.transpose(tf.constant(arr)))
        self.hidden_bias = tfe.Variable(tf.constant(0.0, shape=[self.num_users, self.n_hidden]), name='h_bias')
        self.visible_bias = tfe.Variable(visible_bias, name='v_bias')

        self.weight_v = tf.zeros(tf.shape(self.weights))
        self.visible_bias_v = tf.zeros(tf.shape(self.visible_bias))
        self.hidden_bias_v = tf.zeros(tf.shape(self.hidden_bias))

        # Initialize momentum velocities
        # self.w_v = tfe.Variable(tf.zeros([n_visible, n_hidden, num_rat]), dtype=tf.float32)
		# self.hb_v = tfe.Variable(tf.zeros([n_hidden]), dtype=tf.float32)
		# self.vb_v = tfe.Variable(tf.zeros([n_visible, num_rat]), dtype=tf.float32)

    def forward_prop(self, visible, users):
        '''Computes a vector of probabilities hidden units are set to 1 given
        visible unit states'''
        hidden_bias = tf.batch_gather(self.hidden_bias, users)

        pre_activation = tf.add(tf.tensordot(visible, self.weights, [[1, 2], [1, 0]]), hidden_bias)
        return tf.nn.sigmoid(pre_activation)


    def backward_prop(self, hidden):
        '''Computes a vector of probabilities visible units are set to 1 given
        hidden unit states'''
        return tf.nn.softmax(tf.add(tf.transpose(tf.tensordot(hidden, self.weights, [[1], [2]]), perm=[0, 2, 1]), self.visible_bias), axis=1)

    def sample_h_given_v(self, visible_sample, users, binary=True):
        '''Sample a hidden unit vector based on state probabilities given visible unit states.'''
        hidden_probs = self.forward_prop(visible_sample, users)
        if binary:
        # Relu is just to convert -1 to 0 for sampled vector
            return tf.nn.relu(tf.sign(tf.subtract(hidden_probs, tf.random_uniform(tf.shape(hidden_probs)))))
        else:
            return hidden_probs

    def sample_v_given_h(self, hidden_sample, binary=False):
        '''Sample a visible unit vector based on state probabilities given hidden unit states.'''
        visible_probs = self.backward_prop(hidden_sample)
        # Relu is just to convert -1 to 0 for sampled vector
        if binary:
            return tf.nn.relu(tf.sign(tf.subtract(visible_probs, tf.random_uniform(tf.shape(visible_probs)))))
        else:
            return visible_probs



    def CD_k(self, visibles, mask, users):
        '''Contrastive divergence with k steps of Gibbs Sampling.'''
        orig_hidden = self.sample_h_given_v(visibles, users)
        # k steps of alternating parallel updates
        for i in range(self.k):
            if i == 0:
                hidden_samples = orig_hidden
            visible_samples = self.sample_v_given_h(hidden_samples, binary=False) * mask
            if i == self.k - 1:
                hidden_samples = self.sample_h_given_v(visible_samples, users, binary=False)
            else:
                hidden_samples = self.sample_h_given_v(visible_samples, users)


        w_grad_pos = tf.einsum('ai,ajm->mji', orig_hidden, visibles)

        # Second term, based on reconstruction from Gibbs Sampling

        #user_rated = (5 / (tf.maximum(tf.reduce_sum(mask, axis=(0, 1)), 1)))
        w_neg_grad = tf.einsum('ai,ajm->mji', hidden_samples, visible_samples)
        # w_neg_grad = tf.scalar_mul(1 / self.batch_size, w_neg_grad)
        # w_neg_grad = tf.reduce_sum(tf.transpose(tf.tensordot(visible_samples, hidden_samples, [[0], [0]]), perm=[1, 0, 2]), axis=1)

        w_grad_tot = tf.subtract(w_grad_pos, w_neg_grad)
        # w_grad_tot = tf.einsum('i,ijk->ijk', user_rated, w_grad_tot)

        # Calculate total gradient, accounting for expectation
        # w_grad_tot = tf.stack([w_grad_tot, w_grad_tot, w_grad_tot, w_grad_tot, w_grad_tot], axis=1)
        # Bias gradients
        # hb_grad = tf.reduce_mean(tf.subtract(orig_hidden, hidden_samples), axis=0)



        curr_hidden_bias = tf.batch_gather(self.hidden_bias, users)



        hb_grad = tf.subtract(orig_hidden, hidden_samples)



        tf.scatter_update(self.hidden_bias, users, tf.add(curr_hidden_bias, tf.scalar_mul(self.lr_hb, hb_grad)))





        #hb_grad = tf.reduce_sum(tf.subtract(orig_hidden, hidden_samples), axis=0)

        vb_grad = tf.reduce_sum(tf.subtract(visibles, visible_samples), axis=0)
        # vb_grad = tf.einsum('i,ji->ji', user_rated, vb_grad)

        return w_grad_tot, vb_grad

    def learn(self, visibles, users):

        reduced_visibles = tf.reduce_sum(visibles, axis=1)

        mask = tf.where(tf.equal(reduced_visibles, 0), tf.zeros_like(reduced_visibles), tf.ones_like(reduced_visibles))
        mask = tf.stack([mask, mask, mask, mask, mask], axis=1)

        weight_grad, visible_bias_grad = self.CD_k(visibles, mask, users)

        # return [weight_grad, hidden_bias_grad, visible_bias_grad]
        return [tf.math.negative(weight_grad), tf.math.negative(visible_bias_grad)]

        # Compute new velocities
        # new_w_v = self.momentum * self.w_v + self.lr * weight_grad
        # new_hb_v = self.momentum * self.hb_v + self.lr * hidden_bias_grad
        # new_vb_v = self.momentum * self.vb_v + self.lr * visible_bias_grad

        # new_w_v = self.lr * weight_grad
        # new_hb_v = self.lr * hidden_bias_grad
        # new_vb_v = self.lr * visible_bias_grad
        # Update parameters
        # self.weights = tf.add(self.weights, tf.scalar_mul(-self.lr, weight_grad))
        # self.hidden_bias = tf.add(self.hidden_bias, tf.scalar_mul(-self.lr, hidden_bias_grad))
        # self.visible_bias = tf.add(self.visible_bias, tf.scalar_mul(-self.lr, visible_bias_grad))
        # Update velocities
        # update_w_v = tf.assign(self.w_v, new_w_v)
        # update_hb_v = tf.assign(self.hb_v, new_hb_v)
        # update_vb_v = tf.assign(self.vb_v, new_vb_v)
        #return [update_w, update_hb, update_vb]

    def get_variables(self):
        return [self.hidden_bias, self.visible_bias, self.weights]

    def apply_gradients(self, grads):
        self.weight_v = tf.add(grads[0], tf.scalar_mul(self.momentum, self.weight_v))

        # weight_update -= tf.scalar_mul(self.weight_decay, self.weights)
        tf.assign(self.weights, tf.add(self.weights, tf.scalar_mul(self.lr_weights, self.weight_v)))

        self.hidden_bias_v = tf.add(grads[1], tf.scalar_mul(self.momentum, self.hidden_bias_v))
        tf.assign(self.hidden_bias, tf.add(self.hidden_bias, tf.scalar_mul(self.lr_hb, self.hidden_bias_v)))

        self.visible_bias_v = tf.add(grads[2], tf.scalar_mul(self.momentum, self.visible_bias_v))
        tf.assign(self.visible_bias, tf.add(self.visible_bias, tf.scalar_mul(self.lr_vb, self.visible_bias_v)))


        # vb_update = tf.add(tf.scalar_mul(self.lr_vb, grads[2]), tf.scalar_mul(self.momentum, self.visible_bias_v))
        # vb_update -= tf.scalar_mul(self.weight_decay, self.visible_bias)

        # self.visible_bias_v = vb_update
        # tf.assign(self.visible_bias, tf.add(self.visible_bias, vb_update))

    def train(self, dataset, epochs, probe_set, probe_train):
        # Computation graph definition
        batched_dataset = dataset.batch(self.batch_size)
        iterator = batched_dataset.make_one_shot_iterator()
        optimizer = tf.contrib.opt.MomentumWOptimizer(0.001, 0.01 / self.batch_size, 0.9)

        for epoch in range(epochs):
            if self.anneal:
                self.lr_weights = self.lr_weights / (1 + epoch / self.anneal_val)
                self.lr_vb = self.lr_vb / (1 + epoch / self.anneal_val)
                self.lr_hb = self.lr_hb / (1 + epoch / self.anneal_val)


            if epoch == 42:
                self.k = 3

            num_pts = 0

            print()
            print("Epoch " + str(epoch + 1) + "/" + str(epochs))
            prog_bar = tf.keras.utils.Progbar(460000, stateful_metrics=["val_rmse"])

            try:
                while True:
                    training_point = iterator.get_next()
                    x = tf.sparse.to_dense(training_point[1], default_value = -1)
                    user = training_point[0]

                    x_hot = tf.one_hot(x, self.num_rat, axis=1)
                    grads = self.learn(x_hot, user)
                    optimizer.apply_gradients(zip(grads, [self.weights, self.visible_bias]))
                    # self.apply_gradients(grads)
                    num_pts += 1
                    train_rmse = tf.sqrt( tf.scalar_mul(1 / tf.to_float(tf.count_nonzero(tf.add(x, 1))),
                        tf.reduce_sum(tf.square(tf.subtract(self.forward(x_hot, user), tf.to_float(tf.add(x, 1)))))))
                    prog_bar.update(self.batch_size * num_pts, [("train_rmse", train_rmse)])
            except tf.errors.OutOfRangeError:
                ds = dataset.shuffle(460000)
                batched_dataset = ds.batch(self.batch_size)
                iterator = batched_dataset.make_one_shot_iterator()



            predictions, RMSE = self.pred_with_RMSE(probe_set, probe_train)
            prog_bar.update(self.batch_size * num_pts, [("val_rmse", RMSE)])

            if (epoch + 1) % 5 == 0:
                saver = tf.contrib.eager.Saver(self.get_variables())
                saver.save("rbm_" + str(epoch))

        saver = tf.contrib.eager.Saver(self.get_variables())
        saver.save("rbm")

    def pred_for_sub(self, test_set, pred_set, submit=True, filename="rbm.txt"):
        test_set = test_set.repeat(1)
        test_set = test_set.batch(self.batch_size)
        test_iterator = test_set.make_one_shot_iterator()
        pred_set = pred_set.batch(self.batch_size)
        pred_iterator = pred_set.make_one_shot_iterator()

        training_point = pred_iterator.get_next()
        x = tf.sparse.to_dense(training_point[1], default_value = -1)
        user = training_point[0]
        x_hot = tf.one_hot(x, self.num_rat, axis=1)
        batch_count = 0
        curr_preds = self.forward(x_hot, user, False)


        batch_count = 0
        total_predictions = []
        actual = []
        try:
            while True:
                row_batch = test_iterator.get_next()
                test_preds = tf.gather_nd(curr_preds, row_batch[1].indices)

                total_predictions = tf.concat([total_predictions, test_preds], 0)
                training_point = pred_iterator.get_next()
                x = tf.sparse.to_dense(training_point[1], default_value = -1)
                user = training_point[0]
                x_hot = tf.one_hot(x, self.num_rat, axis=1)
                curr_preds = self.forward(x_hot, user, False)

        except tf.errors.OutOfRangeError:
            pass

        if submit:
            submission = total_predictions.numpy()
            np.savetxt(filename, submission, delimiter="\n")

        return total_predictions

    def pred_with_RMSE(self, test_set, pred_set):
        test_set = test_set.repeat(1)
        test_set = test_set.batch(self.batch_size)
        test_iterator = test_set.make_one_shot_iterator()
        pred_set = pred_set.batch(self.batch_size)
        pred_iterator = pred_set.make_one_shot_iterator()

        training_point = pred_iterator.get_next()
        x = tf.sparse.to_dense(training_point[1], default_value = -1)
        user = training_point[0]
        x_hot = tf.one_hot(x, self.num_rat, axis=1)
        batch_count = 0
        curr_preds = self.forward(x_hot, user, False)


        batch_count = 0
        total_predictions = []
        actual = []
        try:
            while True:
                batch_count += 1

                row_batch = test_iterator.get_next()
                test_preds = tf.gather_nd(curr_preds, row_batch[1].indices)

                total_predictions = tf.concat([total_predictions, test_preds], 0)
                actual = tf.concat([actual, row_batch[1].values], 0)
                batch_count += self.batch_size
                training_point = pred_iterator.get_next()
                x = tf.sparse.to_dense(training_point[1], default_value = -1)
                user = training_point[0]
                x_hot = tf.one_hot(x, self.num_rat, axis=1)
                curr_preds = self.forward(x_hot, user, False)

        except tf.errors.OutOfRangeError:
            pass

        total_error = tf.math.sqrt(tf.losses.mean_squared_error(total_predictions, actual))

        return total_predictions, total_error


    def forward(self, visibles, users, should_mask=True):
        hidden_samples = self.sample_h_given_v(visibles, users, False)
        visible_samples = self.sample_v_given_h(hidden_samples)

        reduced_visibles = tf.reduce_sum(visibles, axis=1)

        scale = tf.constant([[1.0, 2.0, 3.0, 4.0, 5.0]])

        if not should_mask:
            return tf.tensordot(scale, visible_samples, [[1], [1]])[0]

        mask = tf.where(tf.equal(reduced_visibles, 0), tf.zeros_like(reduced_visibles), tf.ones_like(reduced_visibles))
        mask = tf.stack([mask, mask, mask, mask, mask], axis=1)

        return tf.tensordot(scale, mask * visible_samples, [[1], [1]])[0]
