#include "../data_processing/data.h"
#include <armadillo>

using namespace arma;

typedef struct {
    int M;
    int N;
    int K;
    double k_eta;
    double k_reg;
    Data Y;
    Data Y_test;
    Data Y_valid;
    Data Y_all;
    double max_epochs;
} ModelParams;

class Model {

    ModelParams params;
    Mat<double> U;
    Mat<double> V;
    Col<double> del_U;
    Col<double> del_V;
    Col<double> del_alpha_uk;
    //Col<double> del_p_u_kt;
    Col<double> b_i;
    Col<double> b_u;
    Mat<double> b_bin;
    Col<double> c_u;
    Mat<double> c_ut;
    Mat<double> b_u_tui;
    Mat<double> f_ui;
    Mat<double> b_f_ui;
    Col<double> t_u;
    Mat<double> alpha_uk;
    //Cube<double> p_u_kt;
    Col<double> alpha_u;
    vector<vector<int>> N_u;
    Col<int> N_u_size;
    vector<vector<int>> R_u;
    Col<int> R_u_size;
    Mat<double> Y;
    Mat<double> Y_norm;
    Mat<int> Ratings;
    Mat<int> Times;
    vector<vector<tuple<int, int>>> Rating_Time;
    //vector<vector<int>> Ratings_vec;

    void movies_per_user();
    void implicit_movies_per_user();
    void user_frequency();
    void user_date_avg();

    double predict_rating(int user, int movie, int time);

    double devUser(
        int time,
        int user_avg
    );

    void compute_y_norm(int user);

    void update_y_vectors(
        int user,
        Col<double>* Vj,
        int e);


    void grad_U(
        double del_common,
        Col<double> *Ui,
        Col<double> *Vj,
        int e
        );


    void grad_V(
        double del_common,
        Col<double> *Ui,
        Col<double> *Vj,
        Col<double> *y_norm,
        Col<double> *dev_alpha_uk,
        int e
        );

    double grad_common(
        int user,
        int rating,
        double b_u,
        double b_i,
        double b_bin,
        double b_u_tui,
        double b_f_ui,
        double alpha_u,
        double c_u,
        double c_ut,
        //Col<double> *p_u_kt,
        Col<double> *dev_alpha_uk,
        Col<double> *Ui,
        Col<double> *Vj,
        Col<double> *y_norm
    );

    void grad_alpha_uk(
        double del_common,
        int user,
        int time,
        Col<double>* alpha_uk,
        Col<double>* Vj,
        int e);

    // void grad_p_u_kt(
    //     double del_common,
    //     Col<double> *p_u_kt,
    //     Col<double> *Vj,
    //     int e
    // );

    double grad_alpha_u(
        double del_common,
        int user,
        int time,
        double alpha_u,
        int e);

    double grad_b_u_tui(
        double del_common,
        double b_u_tui, int e
    );

    double grad_b_f_ui(
        double del_common,
        double b_f_ui, int e
    );

    double grad_c_u(
        double del_common,
        double c_u,
        double b_i,
        double b_bin,
        int e
    );

    double grad_c_ut(
        double del_common,
        double c_ut,
        double b_i,
        double b_bin,
        int e
    );

    double grad_b_u(
        double del_common,
        double b_u, int e);


    double grad_b_i(
        double del_common,
        double b_i, double c_u, double c_ut, int e
    );

    double grad_b_bin(
        double del_common,
        double b_bin, double c_u, double c_ut,
        int e
    );


    public:

    Model(
        int M,
        int N,
        int K,
        double eta,
        double reg,
        string train_filename,
        string test_filename,
        string valid_filename,
        string all_filename,
        double max_epochs = 10
     );

    vector<double> predict();
    vector<double> predict_probe();
    double trainErr();
    double validErr();
    void train();
    void save(char *file);
    void writeToFileKNN(string filename, vector<double> preds) ;
    void predict_train();
    ~Model();

};
