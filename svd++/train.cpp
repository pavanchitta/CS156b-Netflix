#include <vector>
#include <stdio.h>

#include "svd_plustime2.h"

using namespace std;

void writeToFile(string filename, vector<double> preds) {
    ofstream out;
    out.open(filename);

    for (auto val: preds) {
        out << val << "\n";
    }
    out.close();
}

int main() {
    string all_filename = "../data/um/all.dta";
    string train_filename = "../data/um/train_probe.dta";
    string test_filename = "../data/um/qual.dta";
    string valid_filename = "../data/um/probe.dta";
    int K = 100;
    double eta = 0.007;
    double reg = 0.01;
    Model m(458293, 17770, K, eta, reg, train_filename, test_filename, valid_filename, all_filename);
    cout << "Starting training with " << K << " factors" << endl;
    m.train();
    vector<double> preds = m.predict_train();
    m.writeToFileKNN("../timesvd_knn_train_probe_preds.txt", preds);
    vector<double> preds2 = m.predict();
    writeToFile("../timesvd_knn_test_preds.txt", preds2);

    // writeToFile("/Users/pavanchitta/CS156b-Netflix/svd_plustime_full_100k_007eta_yahuda_init.txt", preds);
    //
    // if (train_filename != "/Users/pavanchitta/CS156b-Netflix/data/um/train_probe.dta") {
    //     vector<double> probe_preds = m.predict_probe();
    //     writeToFile("/Users/pavanchitta/CS156b-Netflix/svd_probepreds_100k_007eta_yahuda_init.txt", probe_preds);
    // }


    return 0;
}
