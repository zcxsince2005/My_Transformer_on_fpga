#ifndef LENET_PERF_H
#define LENET_PERF_H

typedef struct {
    unsigned int runs;
    unsigned int last_cycles;
    unsigned int best_cycles;
    unsigned long long total_cycles;
    unsigned int last_prediction;
    unsigned int last_label;
    unsigned int last_image;
    unsigned int batch_runs;
    unsigned int last_batch_correct;
    unsigned long long last_batch_cycles;
} LenetPerf;

extern LenetPerf g_lenet_perf;

void lenet_perf_print(void);

#endif
