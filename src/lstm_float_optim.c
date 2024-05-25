#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/model_float.h"
#include <stdbool.h>
//////////////////////////////////////////
//                                      //
//          gsl Implementation          // 
//                                      //
//////////////////////////////////////////

gsl_matrix_float *gsl_matrix_alloc(size_t rows, size_t cols) {

    gsl_matrix_float * res= (gsl_matrix_float *)malloc(sizeof(gsl_matrix_float));
    res->size1 = rows;
    res->size2 = cols;
    res->data = (float *)malloc(rows * cols * sizeof(float));

    return res;
}

gsl_matrix_float *gsl_matrix_calloc(size_t rows, size_t cols) {

    gsl_matrix_float * res= (gsl_matrix_float *)malloc(sizeof(gsl_matrix_float));
    res->size1 = rows;
    res->size2 = cols;
    res->data = (float *)calloc(rows * cols,  sizeof(float));

    return res;
}

void gsl_matrix_set(gsl_matrix_float *mat, size_t i, size_t j, float val) {
    mat->data[i * mat->size2 + j] = val;
}

void gsl_matrix_free(gsl_matrix_float *mat) {
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    free(mat->data);
    free(mat);
}


static float gsl_matrix_get(gsl_matrix_float *mat, size_t row, size_t col) {
    float res = mat->data[row * mat->size2 + col];

    return res;
}


static void gsl_matrix_memcpy(gsl_matrix_float *res, gsl_matrix_float *mat) {
    res->size1 = mat->size1;
    res->size2 = mat->size2;
    for (int i=0; i<mat->size1; i++) {
        for (int j=0; j<mat->size2; j++) {
            res->data[i * res->size2 + j] = mat->data[i * mat->size2 + j];
        }
    } 
}


static void gsl_matrix_add(gsl_matrix_float *res, gsl_matrix_float *mat) {
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    for (int i=0; i<mat->size1; i++) {
        for (int j=0; j<mat->size2; j++) {
            res->data[i * cols + j] += mat->data[i * cols + j];
        }
    } 
}




static void gsl_matrix_transpose_memcpy(gsl_matrix_float *res, gsl_matrix_float *mat) {
    res->size1 = mat->size2;
    res->size2 = mat->size1;
    for (int i=0; i<mat->size1; i++) {
        for (int j=0; j<mat->size2; j++) {
            res->data[j * res->size2 + i] = mat->data[i * mat->size2 + j];
        }
    } 
}


static void gsl_blas_dgemm(gsl_matrix_float *A, gsl_matrix_float *B, gsl_matrix_float *C) {
    size_t aRows = A->size1;
    size_t aCols = A->size2;
    size_t bRows = B->size1;
    size_t bCols = B->size2;

    if (aCols != bRows) {
        printf("Matrix mult dimension mismatch\n");
    }

    for (int i = 0; i < aRows; i++) {
        for (int j = 0; j < bCols; j++) {
            float tmp = 0;
            for (int k = 0; k < aCols; k++) {
                tmp += A->data[i * aCols + k] * B->data[k * bCols + j];
            }
            C->data[i * bCols + j] = tmp; 
        }
    }
}




//////////////////////////////////////////
//                                      //
//          Matrix  Utilities           // 
//                                      //
//////////////////////////////////////////

//gsl_matrix_alloc(rows, cols);
//gsl_matrix_free(C);
//gsl_matrix_get(m, 2, 3);
//gsl_matrix_memcpy(result, m1);
//gsl_matrix_add(result, m2);
//gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A, B, 0.0, C);
//gsl_matrix_set(A, i, j, 2.0 * (i + 1) * (j + 1))
//gsl_matrix_transpose_memcpy(x, get_row(xs, i))


void print_matrix_float(gsl_matrix_float *m) {
    size_t rows = m->size1;
    size_t cols = m->size2;
    for (size_t i = 0; i < rows; i++) {
        if (i == 0) printf("[");
        for (size_t j = 0; j < cols; j++) {
            if (j == 0) printf("[ ");
            printf("%f ", m->data[i * cols + j]);
        }
        printf("]");
        if (i != rows-1) printf("\n");
    }
    printf("]\n");
}

float sigmoid(float x) {
    return 1.0 / (1.0 + exp(-x));
}


//sigmoid for matrix
static gsl_matrix_float *sigmoid_mat(gsl_matrix_float *mat) {
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    gsl_matrix_float *result = gsl_matrix_alloc(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            gsl_matrix_set(result, i, j, sigmoid(gsl_matrix_get(mat, i, j)));
        }
    }    
    
    return result;
}

//tanh for matrix
static gsl_matrix_float *tanh_mat(gsl_matrix_float *mat) {
    size_t rows = mat->size1;
    size_t cols = mat->size2;
    gsl_matrix_float *result = gsl_matrix_alloc(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            gsl_matrix_set(result, i, j, tanh(gsl_matrix_get(mat, i, j)));
        }
    }    
    
    return result;
}

// matrix multiplication wrapper
static gsl_matrix_float *gsl_matrix_mult(gsl_matrix_float *A, gsl_matrix_float *B) {
    size_t rowA = A->size1;
    size_t colA = A->size2;
    size_t rowB = B->size1;
    size_t colB = B->size2;
    gsl_matrix_float *result = gsl_matrix_calloc(rowA, colB);

    gsl_blas_dgemm(A, B, result);

    return result;
}


// matrix multiplication wrapper
static gsl_matrix_float *gsl_matrix_sum(gsl_matrix_float *A, gsl_matrix_float *B) {
    size_t rowA = A->size1;
    size_t colA = A->size2;
    size_t rowB = B->size1;
    size_t colB = B->size2;
    gsl_matrix_float *result = gsl_matrix_alloc(rowA, colB);
    gsl_matrix_memcpy(result, A);
    gsl_matrix_add(result, B);

    return result;
}


// Compute Hadamard product (a.k.a. element-wise product)
static gsl_matrix_float *gsl_matrix_hadamard(gsl_matrix_float *A, gsl_matrix_float *B) {
    size_t rowA = A->size1;
    size_t colA = A->size2;
    size_t rowB = B->size1;
    size_t colB = B->size2;

    gsl_matrix_float *result = gsl_matrix_alloc(rowA, colB);

    for (size_t i = 0; i < rowA; i++) {
        for (size_t j = 0; j < colB; j++) {
            float a = gsl_matrix_get(A, i, j);
            float b = gsl_matrix_get(B, i, j);
            gsl_matrix_set(result, i, j, a * b);
        }
    }

    return result;
}


static gsl_matrix_float *get_row(gsl_matrix_float *m, size_t row) {
    size_t rows = m->size1;
    size_t cols = m->size2;
    gsl_matrix_float *result = gsl_matrix_alloc(1, cols);

    for (size_t j = 0; j < cols; j++) {
        float val = gsl_matrix_get(m, row, j);
        gsl_matrix_set(result, 0, j, val);
    }
    return result;
}

static gsl_matrix_float *get_col(gsl_matrix_float *m, size_t col) {
    size_t rows = m->size1;
    size_t cols = m->size2;
    gsl_matrix_float *result = gsl_matrix_alloc(rows, 1);

    for (size_t i = 0; i < rows; i++) {
        float val = gsl_matrix_get(m, i, col);
        gsl_matrix_set(result, i, 0, val);
    }
    return result;
}


//////////////////////////////////////////
//                                      //
//            LSTM  Foward              // 
//                                      //
//////////////////////////////////////////


// Compute common intermediate results for multiple cells before activation
static gsl_matrix_float *wb(gsl_matrix_float *wi, gsl_matrix_float *wh, gsl_matrix_float *bi, gsl_matrix_float *bh, gsl_matrix_float *x, gsl_matrix_float *h) {
    
    gsl_matrix_float *wi_x = gsl_matrix_mult(wi, x);
    gsl_matrix_float *wh_h = gsl_matrix_mult(wh, h);
    gsl_matrix_float *sum1 = gsl_matrix_sum(wi_x, bi);
    gsl_matrix_float *sum2 = gsl_matrix_sum(sum1, wh_h);
    gsl_matrix_float *sum3 = gsl_matrix_sum(sum2, bh);

    gsl_matrix_free(wi_x);
    gsl_matrix_free(wh_h);
    gsl_matrix_free(sum1);
    gsl_matrix_free(sum2);
    
    return sum3;
}

// x has shape [d, 1] (if x is input data) or shape [h, 1] (if x is hidden state from the last layer)
static gsl_matrix_float *one_layer_forward(LSTM_float *lstm, LayerState_float *state, gsl_matrix_float *x, int layer) {
    gsl_matrix_float *h = state->hs[layer];
    gsl_matrix_float *c = state->cs[layer];
    
    gsl_matrix_float *wii = lstm->wiis[layer];
    gsl_matrix_float *wif = lstm->wifs[layer];
    gsl_matrix_float *wig = lstm->wigs[layer];
    gsl_matrix_float *wio = lstm->wios[layer];
    gsl_matrix_float *bii = lstm->biis[layer];
    gsl_matrix_float *bif = lstm->bifs[layer];
    gsl_matrix_float *big = lstm->bigs[layer];
    gsl_matrix_float *bio = lstm->bios[layer];
    
    gsl_matrix_float *whi = lstm->whis[layer];
    gsl_matrix_float *whf = lstm->whfs[layer];
    gsl_matrix_float *whg = lstm->whgs[layer];
    gsl_matrix_float *who = lstm->whos[layer];
    gsl_matrix_float *bhi = lstm->bhis[layer];
    gsl_matrix_float *bhf = lstm->bhfs[layer];
    gsl_matrix_float *bhg = lstm->bhgs[layer];
    gsl_matrix_float *bho = lstm->bhos[layer];

    gsl_matrix_float *it_tmp = wb(wii, whi, bii, bhi, x, h);
    gsl_matrix_float *ft_tmp = wb(wif, whf, bif, bhf, x, h);
    gsl_matrix_float *gt_tmp = wb(wig, whg, big, bhg, x, h);
    gsl_matrix_float *ot_tmp = wb(wio, who, bio, bho, x, h);    

    gsl_matrix_float *it = sigmoid_mat(it_tmp);
    gsl_matrix_float *ft = sigmoid_mat(ft_tmp);
    gsl_matrix_float *gt = tanh_mat(gt_tmp);
    gsl_matrix_float *ot = sigmoid_mat(ot_tmp);

    gsl_matrix_float *ftc_tmp = gsl_matrix_hadamard(ft, c);
    gsl_matrix_float *itgt_tmp = gsl_matrix_hadamard(it, gt);

    gsl_matrix_float *ct = gsl_matrix_sum(ftc_tmp, itgt_tmp);
    gsl_matrix_float *tanhct_tmp = tanh_mat(ct);
    gsl_matrix_float *ht = gsl_matrix_hadamard(ot, tanhct_tmp);

    gsl_matrix_free(state->hs[layer]);
    gsl_matrix_free(state->cs[layer]);

    state->hs[layer] = ht;
    state->cs[layer] = ct;

    gsl_matrix_free(it_tmp);
    gsl_matrix_free(ft_tmp);
    gsl_matrix_free(gt_tmp);
    gsl_matrix_free(ot_tmp);
    gsl_matrix_free(it);
    gsl_matrix_free(ft);
    gsl_matrix_free(gt);
    gsl_matrix_free(ot);
    gsl_matrix_free(ftc_tmp);
    gsl_matrix_free(itgt_tmp);
    gsl_matrix_free(tanhct_tmp);
    
    return ht;
}

// x has shape [d, 1]
static void one_time_forward(LSTM_float *lstm, gsl_matrix_float *x, LayerState_float *state) {
    gsl_matrix_float *x_input = x;

    for (int layer = 0; layer < lstm->num_layers; layer++) {
        x_input = one_layer_forward(lstm, state, x_input, layer);
    }

}


static void initialize_layer_state(LayerState_float *state, int num_layers, int hidden_dim) {
    state->hs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    state->cs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));

    for (int i = 0; i < num_layers; i++) {
        state->hs[i] = gsl_matrix_calloc(hidden_dim, 1);
        state->cs[i] = gsl_matrix_calloc(hidden_dim, 1);
    }
}

static void free_layer_state(LayerState_float *state, int num_layers) {
    for (int i = 0; i < num_layers; i++) {
        gsl_matrix_free(state->hs[i]);
        gsl_matrix_free(state->cs[i]);
    }
    free(state->hs);
    free(state->cs);
    free(state);
}


// Given the input xs with shape [T, d], return the output logit of shape [num_class, 1]
gsl_matrix_float *lstm_forward_float(LSTM_float *lstm, gsl_matrix_float *xs) {
    LayerState_float *state = (LayerState_float *)malloc(sizeof(LayerState_float));
    initialize_layer_state(state, lstm->num_layers, lstm->hidden_dim);

    int T = xs->size1;
    int d = xs->size2;

    gsl_matrix_float *x = gsl_matrix_alloc(d, 1);
    
    for (int i = 0; i < T; i++) {
        gsl_matrix_float *x_row = get_row(xs, i);
        gsl_matrix_transpose_memcpy(x, x_row);
        gsl_matrix_free(x_row);
        one_time_forward(lstm, x, state);
    }

    gsl_matrix_float *h = gsl_matrix_alloc(lstm->hidden_dim, 1);
    gsl_matrix_memcpy(h, state->hs[lstm->num_layers-1]);
    gsl_matrix_float *proj_tmp = gsl_matrix_mult(lstm->w_proj, h);
    gsl_matrix_float *out = gsl_matrix_sum(proj_tmp, lstm->b_proj);
    // clean up memory
    gsl_matrix_free(x);
    gsl_matrix_free(h);
    gsl_matrix_free(proj_tmp);
    free_layer_state(state, lstm->num_layers);

    return out;
}



//////////////////////////////////////////
//                                      //
//      Tensor Reading Unitilities      // 
//                                      //
//////////////////////////////////////////


// Read gsl_matrix_float from a binary file containing a float tensor
static gsl_matrix_float *read_tensor(const char path[], int rows, int cols) {
    FILE *file;

    float *tmp = (float *)malloc(rows * cols * sizeof(float));

    // Open the file for reading
    file = fopen(path, "rb");

    // Read the data into the array
    size_t itemsRead = fread(tmp, sizeof(float), rows * cols, file);

    if (itemsRead == 0) {
        if (ferror(file)) {
            perror("Error reading in matrix");
            fclose(file);
        }
    }

    // Close the file
    fclose(file);

    // convert to matrix
    gsl_matrix_float *mat = gsl_matrix_alloc(rows, cols);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            gsl_matrix_set(mat, i, j, tmp[i * cols + j]);
        }
    }

    // Clean up memory
    free(tmp);

    return mat;
}

// 
static gsl_matrix_float *read_tensor_from_file(FILE* file, int rows, int cols) {

    float *tmp = (float *)malloc(rows * cols * sizeof(float));

    // Read the data into the array
    size_t itemsRead = fread(tmp, sizeof(float), rows * cols, file);
    if (itemsRead == 0) {
        if (ferror(file)) {
            perror("Error reading in matrix");
            free(tmp);
            return NULL;
        }
    }

    // convert to matrix
    gsl_matrix_float *mat = gsl_matrix_alloc(rows, cols);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            gsl_matrix_set(mat, i, j, tmp[i * cols + j]);
        }
    }

    // Clean up memory
    free(tmp);

    return mat;
}


static void write_matrix_float(gsl_matrix_float *mat, FILE* dest_file) {
    size_t rows = mat -> size1;
    size_t cols = mat -> size2;
    float* tmp = (float*) malloc(rows * cols * sizeof(float));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            tmp[i * cols +j] = gsl_matrix_get(mat, i, j);
        }
    }
    fwrite(tmp, sizeof(float), rows * cols, dest_file);
}

void store_lstm_to_single_file(LSTM_float* lstm, FILE* dest_file) {
    for (int i = 0; i < lstm->num_layers; i++) {
        write_matrix_float(lstm->wiis[i], dest_file);
        write_matrix_float(lstm->wifs[i], dest_file);
        write_matrix_float(lstm->wigs[i], dest_file);
        write_matrix_float(lstm->wios[i], dest_file);
        
        write_matrix_float(lstm->biis[i], dest_file);
        write_matrix_float(lstm->bifs[i], dest_file);
        write_matrix_float(lstm->bigs[i], dest_file);
        write_matrix_float(lstm->bios[i], dest_file);

        write_matrix_float(lstm->whis[i], dest_file);
        write_matrix_float(lstm->whfs[i], dest_file);
        write_matrix_float(lstm->whgs[i], dest_file);
        write_matrix_float(lstm->whos[i], dest_file);

        write_matrix_float(lstm->bhis[i], dest_file);
        write_matrix_float(lstm->bhfs[i], dest_file);
        write_matrix_float(lstm->bhgs[i], dest_file);
        write_matrix_float(lstm->bhos[i], dest_file);
    }
    write_matrix_float(lstm -> w_proj, dest_file);
    write_matrix_float(lstm -> b_proj, dest_file);
    //free(dest_file);
}




LSTM_float *load_from_single_file(FILE* file, int num_layers, int input_dim, int hidden_dim, int output_dim) {
    LSTM_float *lstm = (LSTM_float *)malloc(sizeof(LSTM_float));
    lstm->num_layers = num_layers;
    lstm->input_dim = input_dim;
    lstm->hidden_dim = hidden_dim;
    lstm->output_dim = output_dim;

    lstm->wiis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wifs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wigs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wios = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->biis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bifs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bigs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bios = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    
    lstm->whis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whfs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whgs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whos = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhfs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhgs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhos = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));

    for (int i = 0; i < num_layers; i++) {
        if (i == 0) {
            lstm->wiis[i] = read_tensor_from_file(file, hidden_dim, input_dim);
            lstm->wifs[i] = read_tensor_from_file(file, hidden_dim, input_dim);
            lstm->wigs[i] = read_tensor_from_file(file, hidden_dim, input_dim);
            lstm->wios[i] = read_tensor_from_file(file, hidden_dim, input_dim);
        }
        else {
            lstm->wiis[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
            lstm->wifs[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
            lstm->wigs[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
            lstm->wios[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
        }
        
        lstm->biis[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bifs[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bigs[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bios[i] = read_tensor_from_file(file, hidden_dim, 1);
        
        lstm->whis[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
        lstm->whfs[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
        lstm->whgs[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
        lstm->whos[i] = read_tensor_from_file(file, hidden_dim, hidden_dim);
        
        lstm->bhis[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bhfs[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bhgs[i] = read_tensor_from_file(file, hidden_dim, 1);
        lstm->bhos[i] = read_tensor_from_file(file, hidden_dim, 1);

    }
    lstm->w_proj = read_tensor_from_file(file, output_dim, hidden_dim);
    lstm->b_proj = read_tensor_from_file(file, output_dim, 1);
    //free(file);
    return lstm;
}





LSTM_float *get_lstm_float(char* src, int num_layers, int input_dim, int hidden_dim, int output_dim) {
    LSTM_float *lstm = (LSTM_float *)malloc(sizeof(LSTM_float));

    lstm->num_layers = num_layers;
    lstm->input_dim = input_dim;
    lstm->hidden_dim = hidden_dim;
    lstm->output_dim = output_dim;

    lstm->wiis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wifs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wigs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->wios = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->biis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bifs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bigs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bios = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    
    lstm->whis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whfs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whgs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->whos = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhis = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhfs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhgs = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));
    lstm->bhos = (gsl_matrix_float **)malloc(num_layers * sizeof(gsl_matrix_float *));


    printf("start loading model\n");
    char* fname = malloc(500);
    for (int i = 0; i < num_layers; i++) {
        if (i == 0) {
            sprintf(fname, "%s/wii%d.bin", src, i); 
            lstm->wiis[i] = read_tensor(fname, hidden_dim, input_dim);

            sprintf(fname, "%s/wif%d.bin",src, i); 
            lstm->wifs[i] = read_tensor(fname, hidden_dim, input_dim);
            
            sprintf(fname, "%s/wig%d.bin", src, i); 
            lstm->wigs[i] = read_tensor(fname, hidden_dim, input_dim);
            
            sprintf(fname, "%s/wio%d.bin", src, i); 
            lstm->wios[i] = read_tensor(fname, hidden_dim, input_dim);
        }
        else {
            sprintf(fname, "%s/wii%d.bin", src, i); 
            lstm->wiis[i] = read_tensor(fname, hidden_dim, hidden_dim);

            sprintf(fname, "%s/wif%d.bin", src, i); 
            lstm->wifs[i] = read_tensor(fname, hidden_dim, hidden_dim);
            
            sprintf(fname, "%s/wig%d.bin", src, i); 
            lstm->wigs[i] = read_tensor(fname, hidden_dim, hidden_dim);
            
            sprintf(fname, "%s/wio%d.bin", src, i); 
            lstm->wios[i] = read_tensor(fname, hidden_dim, hidden_dim);
        }
        
        
        sprintf(fname, "%s/bii%d.bin", src, i); 
        lstm->biis[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/bif%d.bin", src, i); 
        lstm->bifs[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/big%d.bin", src, i); 
        lstm->bigs[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/bio%d.bin", src, i); 
        lstm->bios[i] = read_tensor(fname, hidden_dim, 1); 
        
        
        sprintf(fname, "%s/whi%d.bin", src, i); 
        lstm->whis[i] = read_tensor(fname, hidden_dim, hidden_dim);
        
        sprintf(fname, "%s/whf%d.bin", src, i); 
        lstm->whfs[i] = read_tensor(fname, hidden_dim, hidden_dim);
        
        sprintf(fname, "%s/whg%d.bin", src,i); 
        lstm->whgs[i] = read_tensor(fname, hidden_dim, hidden_dim);
        
        sprintf(fname, "%s/who%d.bin", src, i); 
        lstm->whos[i] = read_tensor(fname, hidden_dim, hidden_dim);
        
        sprintf(fname, "%s/bhi%d.bin", src,i); 
        lstm->bhis[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/bhf%d.bin", src,i); 
        lstm->bhfs[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/bhg%d.bin", src, i); 
        lstm->bhgs[i] = read_tensor(fname, hidden_dim, 1); 
        
        sprintf(fname, "%s/bho%d.bin", src, i); 
        lstm->bhos[i] = read_tensor(fname, hidden_dim, 1); 
    }
    sprintf(fname, "%s/w_proj.bin", src);
    lstm->w_proj = read_tensor(fname, output_dim, hidden_dim);
    sprintf(fname, "%s/b_proj.bin", src);
    lstm->b_proj = read_tensor(fname, output_dim, 1);
    free(fname);
    return lstm;
}

bool compare_gsl_matrix(gsl_matrix_float* matrix1, gsl_matrix_float* matrix2) {
    if (matrix1->size1 != matrix2->size1) {
        return false;
    }
    if (matrix1->size2 != matrix1->size2) {
        return false;
    }
    size_t rows = matrix1 -> size1;
    size_t cols = matrix1 -> size2; 
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix1->data[i * cols + j] != matrix2->data[i * cols + j]) {
                return false;
            }
        }
    }
    return true;
}

bool compare_lstm_models(LSTM_float* lstm1, LSTM_float* lstm2) {
    if (lstm1 -> num_layers != lstm2 -> num_layers) {
        return false;
    }
    int layers = lstm1 -> num_layers;
    for (int i = 0; i < layers; i++) {
        if (compare_gsl_matrix(lstm1->wiis[i], lstm2 -> wiis[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->wifs[i], lstm2 -> wifs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->wigs[i], lstm2 -> wigs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->wios[i], lstm2 -> wios[i]) == false) {
            return false;
        }

        if (compare_gsl_matrix(lstm1->biis[i], lstm2 -> biis[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bifs[i], lstm2 -> bifs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bigs[i], lstm2 -> bigs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bios[i], lstm2 -> bios[i]) == false) {
            return false;
        }

        if (compare_gsl_matrix(lstm1->whis[i], lstm2 -> whis[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->whfs[i], lstm2 -> whfs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->whgs[i], lstm2 -> whgs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->whos[i], lstm2 -> whos[i]) == false) {
            return false;
        }

        if (compare_gsl_matrix(lstm1->bhis[i], lstm2 -> bhis[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bhfs[i], lstm2 -> bhfs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bhgs[i], lstm2 -> bhgs[i]) == false) {
            return false;
        }
        if (compare_gsl_matrix(lstm1->bhos[i], lstm2 -> bhos[i]) == false) {
            return false;
        }
    }
    if (compare_gsl_matrix(lstm1->w_proj, lstm2 -> w_proj) == false) {
        return false;
    }
    if (compare_gsl_matrix(lstm1->b_proj, lstm2 -> b_proj) == false) {
        return false;
    }
    return true;
}


static void free_matrix_array(gsl_matrix_float **marray, size_t len) {
    for (int i=0; i<len; i++) {
        gsl_matrix_free(marray[i]);
    }
    free(marray);
}

void free_lstm_float(LSTM_float *lstm) {
    free_matrix_array(lstm->wiis, lstm->num_layers);
    free_matrix_array(lstm->wifs, lstm->num_layers);
    free_matrix_array(lstm->wigs, lstm->num_layers);
    free_matrix_array(lstm->wios, lstm->num_layers);
    free_matrix_array(lstm->biis, lstm->num_layers);
    free_matrix_array(lstm->bifs, lstm->num_layers);
    free_matrix_array(lstm->bigs, lstm->num_layers);
    free_matrix_array(lstm->bios, lstm->num_layers);
    
    free_matrix_array(lstm->whis, lstm->num_layers);
    free_matrix_array(lstm->whfs, lstm->num_layers);
    free_matrix_array(lstm->whgs, lstm->num_layers);
    free_matrix_array(lstm->whos, lstm->num_layers);
    free_matrix_array(lstm->bhis, lstm->num_layers);
    free_matrix_array(lstm->bhfs, lstm->num_layers);
    free_matrix_array(lstm->bhgs, lstm->num_layers);
    free_matrix_array(lstm->bhos, lstm->num_layers);
    
    gsl_matrix_free(lstm->w_proj);
    gsl_matrix_free(lstm->b_proj);

    free(lstm);
}



int generate_label(gsl_matrix_float* vec) {
    if (vec == NULL) {
        return -1; // Return -1 if array is empty or invalid size
    }
    int max_index = 0;
    float max_value = gsl_matrix_get(vec, 0, 0);

    for (int i = 1; i < vec->size1; i++) {
        float current_val = gsl_matrix_get(vec, i, 0);
        if (current_val > max_value) {
            max_value = current_val;
            max_index = i;
        }
    }

    return max_index;
}


// int main() {
//     int num_layers = 5;
//     int input_dim = 6;
//     int hidden_dim = 100;
//     int output_dim = 10;
//     char* src = "lstm_bin_float";
//     char* tmp_stored = "lstm.bin";
//     printf("loading models\n");
    
//     FILE* storage;
//     storage = fopen(tmp_stored, "wb");
//     LSTM_float *lstm = get_lstm_float(src, num_layers, input_dim, hidden_dim, output_dim);
//     printf("saving model\n");
//     store_lstm_to_single_file(lstm, storage);
//     fclose(storage);

//     printf("start reloading\n");
    
//     FILE* lstm_file;
//     lstm_file = fopen(tmp_stored, "rb");
//     LSTM_float *reload = load_from_single_file(lstm_file, num_layers, input_dim, hidden_dim, output_dim);
//     if (compare_lstm_models(lstm, reload) == false) {
//         printf("Invalid storing and loading\n");
//     } else {
//         printf("reloading test successful\n");
//     }
//     fclose(lstm_file);
//     return 0;

// }


// int main() {
//     int num_layers = 5;
//     int input_dim = 6;
//     int hidden_dim = 100;
//     int output_dim = 10;
//     printf("here\n");
//     char* src = "model/model_bin_float";
//     LSTM_float *lstm = get_lstm_float(src, num_layers, input_dim, hidden_dim, output_dim);
    
//     printf("num_layers: %d\n", lstm->num_layers);
//     printf("input_dim: %d\n", lstm->input_dim);
//     printf("hidden_dim: %d\n", lstm->hidden_dim);
//     printf("output_dim: %d\n", lstm->output_dim);
//     //printf("whi_l0: \n");
//     //print_matrix(lstm->whis[0]);

//     gsl_matrix_float *xs = gsl_matrix_calloc(10, 6);
//     // fill in input xs value ....
    
//     printf("input_sequence: \n");
//     print_matrix_float(xs);

//     // logits of shape [10, 1]
//     gsl_matrix_float *logits = lstm_forward_float(lstm, xs);
//     printf("output_logits: \n");
//     print_matrix_float(logits);


//     //////////  STRESS TEST  ///////////
//     int request_num = 10000;
//     time_t start, end;

//     time(&start);
//     struct tm *startTime = localtime(&start);
//     printf("Start time: %02d-%02d-%04d %02d:%02d:%02d\n",
//            startTime->tm_mday,           // Day of the month [1-31]
//            startTime->tm_mon + 1,        // Month of the year [0-11, so add 1]
//            startTime->tm_year + 1900,    // Year since 1900, so add 1900
//            startTime->tm_hour,           // Hours since midnight [0-23]
//            startTime->tm_min,            // Minutes after the hour [0-59]
//            startTime->tm_sec);           // Seconds after the minute [0-59]

//     // start testing
//     for (int i=0; i<request_num; i++) {
//         gsl_matrix_float *tmp = lstm_forward_float(lstm, xs);
//         gsl_matrix_free(tmp);
//     }
  

//     time(&end);
//     struct tm *endTime = localtime(&end);
//     printf("End time: %02d-%02d-%04d %02d:%02d:%02d\n",
//            endTime->tm_mday,           // Day of the month [1-31]
//            endTime->tm_mon + 1,        // Month of the year [0-11, so add 1]
//            endTime->tm_year + 1900,    // Year since 1900, so add 1900
//            endTime->tm_hour,           // Hours since midnight [0-23]
//            endTime->tm_min,            // Minutes after the hour [0-59]
//            endTime->tm_sec); 
    

//     // clean up 
//     gsl_matrix_free(logits);
//     gsl_matrix_free(xs);
//     free_lstm_float(lstm);

//     return 0;
// }

