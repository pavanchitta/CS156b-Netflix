#include "matrix_factor_bias.h"
#include <stdio.h>

using namespace std;

Model::Model(
        int M, 
        int N, 
        int K, 
        double eta, 
        double reg, 
        string filename, 
        double mu,
        double eps,
        double max_epochs
     ) : params( { M, N, K, eta, reg, Data(filename), mu, eps, max_epochs } ){
}

Col<double> Model::gradU(Col<double> Ui, int y, Col<double> Vj, double ai, double bj, double s) {
    Col<double> res = this->params.eta * ((this->params.reg * Ui) - Vj * s);
    return res;
}

Col<double> Model::gradV(Col<double> Ui, int y, Col<double> Vj, double ai, double bj, double s) {
    return this->params.eta * ((this->params.reg * Vj) - Ui * s);
} 

double Model::gradA(Col<double> Ui, int y, Col<double> Vj, double ai, double bj, double s) {
    return as_scalar(this->params.eta * ((this->params.reg * ai) - s));
}

double Model::gradB(Col<double> Ui, int y, Col<double> Vj, double ai, double bj, double s) {
    return as_scalar(this->params.eta * ((this->params.reg * bj) - s));
}

double Model::trainErr() {
    vector<vector<int> >::iterator ptr;
    double loss_err = 0.0;


    while (this->params.Y.hasNext()) {
        vector<int> p = this->params.Y.nextLine();
        int i = p[0];
        int j = p[1];
        int y = p[3];

        loss_err += pow((y - this->params.mu - dot(U.col(i - 1), V.col(j - 1))
                - a[i - 1] - b[j - 1]), 2);
    }

    return loss_err;
}

Model::~Model() {}

void Model::train() {
    this->U = Mat<double>(this->params.K, this->params.M, fill::randu);
    this->V = Mat<double>(this->params.K, this->params.N, fill::randu);
    this->a = Col<double>(this->params.M, fill::randu);
    this->b = Col<double>(this->params.N, fill::randu);

    this->U -= 0.5;
    this->V -= 0.5;
    this->a -= 0.5;
    this->b -= 0.5;

    for (int e = 0; e < this->params.max_epochs; e++) {
        cout << "Running Epoch " << e << endl;
        int k = 0;

        while (this->params.Y.hasNext()) {
            vector<int> p = this->params.Y.nextLine();
            int i = p[0];
            int j = p[1];
            int y = p[3];

            Col<double> u = this->U.col(i - 1);
            Col<double> v = this->V.col(j - 1); 

            double s = as_scalar(y - this->params.mu - dot(u, v) - this->a[i - 1] - this->b[j - 1]);

            Col<double> del_U = this->gradU(u, y, v,  
                    this->a[i - 1], this->b[j - 1], s);
            Col<double> del_V = this->gradV(u, y, v, 
                    this->a[i - 1], this->b[j - 1], s);
            double del_A = this->gradA(u, y, v, 
                    this->a[i - 1], this->b[j - 1], s);
            double del_B = this->gradB(u, y, v, 
                    this->a[i - 1], this->b[j - 1], s);

            this->U.col(i - 1) -= del_U;
            this->V.col(j - 1) -= del_V;
            this->a[i - 1] -= del_A;
            this->b[j - 1] -= del_B;

            k++;
            
            if (k % 10000 == 0) {
                cout << k << endl;
            }
        }

        this->params.Y.reset();
        cout << "Error " << trainErr() << endl;
    }
}
