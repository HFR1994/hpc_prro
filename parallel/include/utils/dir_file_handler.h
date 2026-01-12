//
// Created by vscode on 1/8/26.
//

#ifndef RRA_PARALLEL_DATASET_HANDLER_H
#define RRA_PARALLEL_DATASET_HANDLER_H

int parse_dims_from_name(const char *name, int *out_rows,int *out_cols);
int read_dataset_csv(const char *path, double *X, int rows, int cols);
int ensure_dir_exists(const char *path);

#endif //RRA_PARALLEL_DATASET_HANDLER_H