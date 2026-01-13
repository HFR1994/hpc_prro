#include "utils/dir_file_handler.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mpi/wrapper.h"
#include "utils/logger.h"

int ensure_dir_exists(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        // Path exists
        if (S_ISDIR(st.st_mode)) {
            return 0; // already exists
        }
        log_err("%s exists but is not a directory\n", path);
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
int read_dataset_csv(const char *path, prro_state_t * local, const prra_cfg_t global, const mpi_ctx_t *ctx) {

    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    const MPI_Offset line_bytes = CSV_LINE_BYTES(global.features);
    const MPI_Offset offset = (MPI_Offset)local->start_row * line_bytes;
    const MPI_Offset nbytes = (MPI_Offset)local->local_rows * line_bytes;

    char *buffer = malloc(nbytes);
    if (!buffer) {
        log_err("Failed to allocate buffer for CSV read");
        free(buffer);
        return -1;
    }

    // Use MPI aware file functions
    MPI_File fh;
    MPI_CHECK(MPI_File_open(ctx->comm, path, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh));
    MPI_CHECK(MPI_File_read_at_all(fh, offset,buffer, nbytes, MPI_CHAR, MPI_STATUS_IGNORE));
    MPI_CHECK(MPI_File_close(&fh));

    /* Parse fixed-width rows */
    for (int i = 0; i < local->local_rows; ++i) {
        const char *line = buffer + (size_t)i * line_bytes;

        for (int j = 0; j < global.features; ++j) {
            char field[CSV_FIELD_WIDTH + 1];

            const size_t pos = (size_t)j * (CSV_FIELD_WIDTH + CSV_DELIM_BYTES);

            memcpy(field, line + pos, CSV_FIELD_WIDTH);
            field[CSV_FIELD_WIDTH] = '\0';

            local->food_source[i * global.features + j] = strtod(field, NULL);
        }
    }

    free(buffer);

    return local->local_rows; // number of rows read
}
