#include "matrix_factor_bias.h"
#include <stdio.h>

using namespace std;

SVD::SVD(
        int M,
        int N,
        int K,
        double eta,
        double reg,
        string train_filename,
        string test_filename,
        string valid_filename,
        double mu,
        double eps,
        double max_epochs


    ) : params( { M, N, K, eta, reg, Data(train_filename), Data(test_filename), Data(valid_filename), mu, eps, max_epochs}){

}


void SVD::updateGradU(Col<double> *Ui, int y, Col<double> *Vj, double ai, double bj, double s,
        int i, int j) {
    //this->U.col(i - 1) -= this->params.eta * ((this->params.reg * *Ui) - (*Vj) * s);
    this->del_U = this->params.eta * ((this->params.reg * *Ui) - (*Vj) * s);
}

void SVD::updateGradV(Col<double> *Ui, int y, Col<double> *Vj, double ai, double bj, double s,
        int i, int j) {
    //this->V.col(j - 1) -= this->params.eta * ((this->params.reg * *Vj) - *Ui * s);
    this->del_V = this->params.eta * ((this->params.reg * *Vj) - *Ui * s);
}

double SVD::gradA(Col<double> *Ui, int y, Col<double> *Vj, double ai, double bj, double s) {
    return as_scalar(this->params.eta * ((this->params.reg * ai) - s));
}

double SVD::gradB(Col<double> *Ui, int y, Col<double> *Vj, double ai, double bj, double s) {
    return as_scalar(this->params.eta * ((this->params.reg * bj) - s));
}

vector<double> SVD::predict() {
    vector<double> preds;
    while (this->params.Y_test.hasNext()) {
        NetflixData p = this->params.Y_test.nextLine();
        int user = p.user;
        int movie = p.movie;
        Col<double> u = this->U.col(user - 1);
        Col<double> v = this->V.col(movie - 1);
        double pred = this->params.mu + dot(u, v) + this->a[user - 1] + this->b[movie - 1];
        preds.push_back(pred);
    }
    return preds;
}

vector<double> SVD::predict_train() {
    vector<double> preds;
    this->params.Y.reset();
    while (this->params.Y.hasNext()) {
        NetflixData p = this->params.Y.nextLine();
        int user = p.user;
        int movie = p.movie;
        Col<double> u = this->U.col(user - 1);
        Col<double> v = this->V.col(movie - 1);
        double pred = this->params.mu + dot(u, v) + this->a[user - 1] + this->b[movie - 1];
        preds.push_back(pred);
    }
    return preds;
}


vector<double> SVD::predict_probe() {
    vector<double> preds;
    this->params.Y_valid.reset();
    while (this->params.Y_valid.hasNext()) {
        NetflixData p = this->params.Y_valid.nextLine();
        int user = p.user;
        int movie = p.movie;
        Col<double> u = this->U.col(user - 1);
        Col<double> v = this->V.col(movie - 1);
        double pred = this->params.mu + dot(u, v) + this->a[user - 1] + this->b[movie - 1];
        preds.push_back(pred);
    }
    return preds;
}

void SVD::writeToFileKNN(string filename, vector<double> preds) {
    ofstream out;
    out.open(filename);

    int i = 0;

    this->params.Y.reset();
    while (this->params.Y.hasNext()) {
        NetflixData p = this->params.Y.nextLine();
        int user = p.user;
        int movie = p.movie;
        int time = p.date;

        out << setprecision(5) << user << " " << movie << " " << time << " " << preds[i] << "\n";
        i++;
    }
    cout << "reached" << endl;
    // return preds;
    //
    // for (auto val: preds) {
    //     out << val << "\n";
    // }
    out.close();
}

double SVD::trainErr() {
    vector<vector<int> >::iterator ptr;

    int k = 0;
    double loss_err = 0.0;
    while (this->params.Y.hasNext()) {
        NetflixData p = this->params.Y.nextLine();
        int i = p.user;
        int j = p.movie;
        int y = p.rating;
        //cout << "Point " << a << endl;
        loss_err += pow((y - this->params.mu - dot(U.col(i - 1), V.col(j - 1))
                - a[i - 1] - b[j - 1]), 2);

        k++;
    }

    return loss_err / k;
}

double SVD::validErr() {
    int k = 0;
    double loss_err = 0.0;
    while (this->params.Y_valid.hasNext()) {
        NetflixData p = this->params.Y_valid.nextLine();
        int i = p.user;
        int j = p.movie;
        int y = p.rating;
        //cout << "Point " << a << endl;
        loss_err += pow((y - this->params.mu - dot(U.col(i - 1), V.col(j - 1))
                - a[i - 1] - b[j - 1]), 2);

        k++;
    }

    return loss_err / k;
}

SVD::~SVD() {}

void SVD::train() {
    this->U = Mat<double>(this->params.K, this->params.M, fill::randu);
    this->V = Mat<double>(this->params.K, this->params.N, fill::randu);
    this->a = Col<double>(this->params.M, fill::randu);
    this->b = Col<double>(this->params.N, fill::randu);
    this->del_U = Col<double>(this->params.K, fill::zeros);
    this->del_V = Col<double>(this->params.K, fill::zeros);

    // this->U -= 0.5;
    // this->V -= 0.5;
    // this->a -= 0.5;
    // this->b -= 0.5;

    this->U /= 700;
    this->V /= 700;
    this->a /= 700;
    this->b /= 700;

    double prev_err = validErr();
    double curr_err = 0.0;
    for (int e = 0; e < this->params.max_epochs; e++) {
        cout << "Running Epoch " << e << endl;
        int count = 0;
        while (this->params.Y.hasNext()) {
            NetflixData p = this->params.Y.nextLine();
            int i = p.user;
            int j = p.movie;
            int y = p.rating;

            count++;

            Col<double> u = this->U.col(i - 1);
            Col<double> v = this->V.col(j - 1);

            double s = as_scalar(y - this->params.mu - dot(u, v) - this->a[i - 1] - this->b[j - 1]);


            this->updateGradU(&u, y, &v,
                    this->a[i - 1], this->b[j - 1], s, i, j);
            this->updateGradV(&u, y, &v,
                    this->a[i - 1], this->b[j - 1], s, i, j);
            double del_A = this->gradA(&u, y, &v,
                    this->a[i - 1], this->b[j - 1], s);
            double del_B = this->gradB(&u, y, &v,
                    this->a[i - 1], this->b[j - 1], s);
            this->U.col(i - 1) -= this->del_U;
            this->V.col(j - 1) -= this->del_V;

            this->a[i - 1] -= del_A;
            this->b[j - 1] -= del_B;
        }

        cout << "Ran through points" << endl;
        this->params.Y_valid.reset();
        curr_err = validErr();
        cout << "Probe Error " << curr_err << endl;
        this->params.Y_valid.reset();
        this->params.Y.reset();

        //Early stopping
        if (prev_err < curr_err) {
            cout << "Early Stopping" << endl;
            break;
        }

        prev_err = curr_err;
        // cout << "Error " << trainErr() << endl;
        // this->params.Y.reset();
    }
}
