//
// Created by vscode on 1/8/26.
//

#ifndef RRA_PARALLEL_DATASET_HANDLER_H
#define RRA_PARALLEL_DATASET_HANDLER_H
#include "mpi/wrapper.h"

// Use to control the size of the csv file, so it can be parallelize based on the exact number of
#define CSV_FIELD_WIDTH 18   // must match generator.py
#define CSV_DELIM_BYTES 1    // ','
#define CSV_NEWLINE_BYTES 1  // '\n'
#define CSV_LINE_BYTES(cols) ((cols) * CSV_FIELD_WIDTH + ((cols) - 1) * CSV_DELIM_BYTES + CSV_NEWLINE_BYTES)

int parse_dims_from_name(const char *name, int *out_rows,int *out_cols);
int read_dataset_csv(const char *path, prro_state_t * local, const prra_cfg_t global, const mpi_ctx_t *ctx);
int ensure_dir_exists(const char *path);

#endif //RRA_PARALLEL_DATASET_HANDLER_H