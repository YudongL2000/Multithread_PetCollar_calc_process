#ifndef GLOBALS_H
#define GLOBALS_H

#define BUFFLEN 500
#define MSG_LEN 100
#define MAXRELOADS 100
#define MAX_ITERS 500

#define FILESTARTSYMBOL '{'
#define FILEENDSYMBOL '}'


extern int DEFAULT_HISTORY_LEN;
extern int DEFAULT_LSTM_INPUT_LEN;
extern int DEFAULT_LSTM_LAYERS;
extern int DEFAULT_LSTM_INPUT_DIM;
extern int DEFAULT_LSTM_HIDDEN_DIM;
extern int DEFAULT_CNT_GAP_TOLERANCE;

extern int DEFAULT_LSTM_OUTPUT_DIM_CAT;
extern int DEFAULT_LSTM_OUTPUT_DIM_DOG;

//if we have more than a certain number of rests, then we directly go to sleep
extern int DEFAULT_CAT_SLEEP_MODE_THRESH;
extern int DEFAULT_DOG_SLEEP_MODE_THRESH;





#endif