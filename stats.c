#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* ---------- Utilities ---------- */

static void die_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <int1> <int2> ...\n", prog);
    fprintf(stderr, "Example: %s 1 2 2 3 4 4 4 5\n", prog);
    exit(EXIT_FAILURE);
}

static int cmp_int(const void *a, const void *b) {
    const int ai = *(const int *)a;
    const int bi = *(const int *)b;
    return (ai > bi) - (ai < bi);
}

/* Parse a single integer from a string with strict checking. */
static int parse_int_strict(const char *s, int *out) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;  // invalid
    if (v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

/* ---------- Core statistics ---------- */

static double mean(const int *a, size_t n) {
    long long sum = 0;              // avoid overflow for moderate n
    for (size_t i = 0; i < n; ++i) {
        sum += a[i];
    }
    return (double)sum / (double)n;
}

/* Assumes 'sorted' is sorted ascending. */
static double median_sorted(const int *sorted, size_t n) {
    if (n % 2 == 1) {
        return (double)sorted[n / 2];
    } else {
        int left = sorted[n / 2 - 1];
        int right = sorted[n / 2];
        return (left + right) / 2.0;
    }
}

/*
  Compute modes from a sorted array.
  - 'modes_out' must have capacity at least n.
  - Returns the number of modes, and writes max frequency to *maxfreq_out.
*/
static size_t modes_from_sorted(const int *sorted, size_t n,
                                int *modes_out, int *maxfreq_out) {
    if (n == 0) { *maxfreq_out = 0; return 0; }

    int maxf = 0;
    size_t modes_count = 0;

    size_t i = 0;
    while (i < n) {
        int value = sorted[i];
        size_t count = 1;
        while (i + count < n && sorted[i + count] == value) ++count;

        if ((int)count > maxf) {
            maxf = (int)count;
            modes_out[0] = value;
            modes_count = 1;
        } else if ((int)count == maxf) {
            modes_out[modes_count++] = value;
        }

        i += count;
    }

    *maxfreq_out = maxf;
    return modes_count;
}

/* ---------- Main ---------- */

int main(int argc, char **argv) {
    if (argc <= 1) {
        die_usage(argv[0]);
    }

    const size_t n = (size_t)(argc - 1);
    int *a = (int *)malloc(n * sizeof(int));
    if (!a) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    // Parse inputs strictly as integers.
    for (size_t i = 0; i < n; ++i) {
        int v;
        if (!parse_int_strict(argv[i + 1], &v)) {
            fprintf(stderr, "Invalid integer: '%s'\n", argv[i + 1]);
            free(a);
            return EXIT_FAILURE;
        }
        a[i] = v;
    }

    // Prepare a sorted copy for median/mode.
    int *sorted = (int *)malloc(n * sizeof(int));
    int *modes = (int *)malloc(n * sizeof(int));
    if (!sorted || !modes) {
        perror("malloc");
        free(a);
        free(sorted);
        free(modes);
        return EXIT_FAILURE;
    }
    memcpy(sorted, a, n * sizeof(int));
    qsort(sorted, n, sizeof(int), cmp_int);

    // Compute stats.
    const double m = mean(a, n);
    const double med = median_sorted(sorted, n);
    int maxfreq = 0;
    const size_t modes_count = modes_from_sorted(sorted, n, modes, &maxfreq);

    // Print results.
    printf("Count : %zu\n", n);
    printf("Mean  : %.6f\n", m);
    printf("Median: %.6f\n", med);
    printf("Mode  : ");
    for (size_t i = 0; i < modes_count; ++i) {
        printf("%d%s", modes[i], (i + 1 < modes_count ? " " : ""));
    }
    printf(" (frequency=%d)\n", maxfreq);

    free(a);
    free(sorted);
    free(modes);
    return EXIT_SUCCESS;
}
