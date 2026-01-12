#include "utils/dir_file_handler.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

int ensure_dir_exists(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        // Path exists
        if (S_ISDIR(st.st_mode)) {
            return 0; // already exists
        }
        fprintf(stderr, "%s exists but is not a directory\n", path);
        return -1;
    }

    // Doesn't exist → try to create
    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return -1;
    }

    return 0;
}

// Checks that files must be named random-<<rows>>-<<cols>>.csv
int parse_dims_from_name(const char *name, int *out_rows,int *out_cols) {
    if (!name || !out_rows || !out_cols)
        return -1;

    const char *last_dash = strrchr(name, '-');
    if (!last_dash) {
        return -1;
    }

    const char *mid = last_dash;
    while (mid > name && *(mid - 1) != '-') {
        mid--;
    }

    if (mid == name) {
        return -1;
    }

    *out_rows = atoi(mid);
    *out_cols = atoi(last_dash + 1);

    if (*out_rows <= 0 || *out_cols <= 0)
        return -1;

    return 0;
}

// Helper: read CSV file at `path` and populate X (row-major).
// Returns number of rows read on success (>=0), or -1 on error (file open/read error).
int read_dataset_csv(const char *path, double *X, const int rows, const int cols) {
    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int row = 0;

    while ((read = getline(&line, &len, f)) != -1 && row < rows) {
        // Remove trailing newline characters
        while (read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) {
            line[--read] = '\0';
        }

        if (read == 0) continue;  // skip empty lines

        // Parse comma-separated tokens
        const char *token = strtok(line, ",");
        int col = 0;
        while (token) {
            if (col+1 > cols) {
                fprintf(stderr,"Row %d doesn't match the expected number of columns %d > %d\n", row, col+1, cols);
                free(line);
                fclose(f);
                return -1;
            }

            X[row * cols + col] = strtod(token, NULL);
            col++;
            token = strtok(NULL, ",");
        }

        row++;
    }

    free(line);
    fclose(f);
    return row; // number of rows read
}
