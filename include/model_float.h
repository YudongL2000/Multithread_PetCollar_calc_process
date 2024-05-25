#ifndef MODEL_FLOAT_H
#define MODEL_FLOAT_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>


typedef struct {
    float *data;
    size_t size1;
    size_t size2;
} gsl_matrix_float;


typedef struct {
    gsl_matrix_float **hs;
    gsl_matrix_float **cs;
} LayerState_float;


typedef struct {
    int num_layers;
    int input_dim;
    int hidden_dim;
    int output_dim;

    gsl_matrix_float **wiis;
    gsl_matrix_float **wifs;
    gsl_matrix_float **wigs;
    gsl_matrix_float **wios;
    gsl_matrix_float **biis;
    gsl_matrix_float **bifs;
    gsl_matrix_float **bigs;
    gsl_matrix_float **bios;
    
    gsl_matrix_float **whis;
    gsl_matrix_float **whfs;
    gsl_matrix_float **whgs;
    gsl_matrix_float **whos;
    gsl_matrix_float **bhis;
    gsl_matrix_float **bhfs;
    gsl_matrix_float **bhgs;
    gsl_matrix_float **bhos;
    
    gsl_matrix_float *w_proj;
    gsl_matrix_float *b_proj;
} LSTM_float;

//matrix operations
gsl_matrix_float *gsl_matrix_alloc(size_t rows, size_t cols);
gsl_matrix_float *gsl_matrix_calloc(size_t rows, size_t cols);
void gsl_matrix_free(gsl_matrix_float *mat);
void gsl_matrix_set(gsl_matrix_float *mat, size_t i, size_t j, float val);


//loading and storing models
LSTM_float *get_lstm_float(char* src, int num_layers, int input_dim, int hidden_dim, int output_dim);
LSTM_float* load_from_single_file(FILE* file, int num_layers, int input_dim, int hidden_dim, int output_dim);
void store_lstm_to_single_file(LSTM_float* lstm, FILE* file);
bool compare_lstm_models(LSTM_float* lstm1, LSTM_float* lstm2);

void free_lstm_float(LSTM_float *lstm);
gsl_matrix_float *lstm_forward_float(LSTM_float *lstm, gsl_matrix_float *xs);
void print_matrix_float(gsl_matrix_float* m);
int generate_label(gsl_matrix_float* vec);


#endif 