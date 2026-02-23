#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#define HASHMAP_BENCH 1
#include "hashmap.h"

typedef struct {
    double min_ns;
    double avg_ns;
    double p95_ns;
} LatencyStats;

static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

static int cmp_ll(const void *a, const void *b) {
    const long long va = *(const long long *)a;
    const long long vb = *(const long long *)b;
    return (va > vb) - (va < vb);
}

static LatencyStats compute_latency(long long *samples, size_t count) {
    LatencyStats stats = {0.0, 0.0, 0.0};
    if (count == 0) {
        return stats;
    }

    long long min_v = samples[0];
    long long sum = 0;
    for (size_t i = 0; i < count; i++) {
        if (samples[i] < min_v) {
            min_v = samples[i];
        }
        sum += samples[i];
    }

    qsort(samples, count, sizeof(long long), cmp_ll);
    size_t p95_index = (size_t)((count - 1) * 0.95);

    stats.min_ns = (double)min_v;
    stats.avg_ns = (double)sum / (double)count;
    stats.p95_ns = (double)samples[p95_index];
    return stats;
}

static size_t choose_sample_every(size_t n) {
    if (n <= 1000) {
        return 1;
    }
    if (n <= 10000) {
        return 10;
    }
    if (n <= 100000) {
        return 100;
    }
    return 1000;
}

static char **build_keys(size_t n) {
    char **keys = malloc(sizeof(char *) * n);
    if (!keys) {
        return NULL;
    }

    for (size_t i = 0; i < n; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "key_%zu", i);
        size_t len = strlen(buf) + 1;
        keys[i] = malloc(len);
        if (!keys[i]) {
            for (size_t j = 0; j < i; j++) {
                free(keys[j]);
            }
            free(keys);
            return NULL;
        }
        memcpy(keys[i], buf, len);
    }

    return keys;
}

static void free_keys(char **keys, size_t n) {
    if (!keys) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        free(keys[i]);
    }
    free(keys);
}

static long long rss_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return -1;
    }
    return usage.ru_maxrss;
}

static void print_header(void) {
    printf("Benchmark HashMap (open addressing)\n");
    printf("Sizes: 1k, 10k, 50k, 100k, 1M, 10M\n\n");
    printf("%-10s %-12s %-15s %-10s %-15s %-12s %-12s\n",
           "Operation", "Elements", "Ops/sec", "Probes/op", "Latency(ns)", "Resizes", "RSS(KB)");
    printf("%-10s %-12s %-15s %-10s %-15s %-12s %-12s\n",
           "----------", "--------", "-------", "----------", "----------", "-------", "--------");
}

static void bench_hash_cost(const char *key, size_t iterations) {
    long long start = now_ns();
    for (size_t i = 0; i < iterations; i++) {
        (void)_hash_func(key, 1024);
    }
    long long elapsed = now_ns() - start;
    double avg_ns = (double)elapsed / (double)iterations;
    printf("hash avg ns: %.2f (iterations=%zu)\n", avg_ns, iterations);
}

static void bench_add(size_t n, char **keys) {
    HashMap map = create_hashmap();
    size_t sample_every = choose_sample_every(n);
    size_t sample_count = (n + sample_every - 1) / sample_every;
    long long *samples = malloc(sizeof(long long) * sample_count);
    size_t sample_index = 0;

    hashmap_bench_reset();
    long long start = now_ns();
    for (size_t i = 0; i < n; i++) {
        long long t0 = 0;
        if ((i % sample_every) == 0) {
            t0 = now_ns();
        }

        add_hashmap(&map, keys[i], (int)i);

        if ((i % sample_every) == 0) {
            samples[sample_index++] = now_ns() - t0;
        }
    }
    long long elapsed = now_ns() - start;

    LatencyStats stats = compute_latency(samples, sample_index);
    double secs = (double)elapsed / 1000000000.0;
    double ops_sec = (double)n / secs;
    long long rss = rss_kb();

    printf("%-10s %-12zu %-15.0f %-10.2f %-15.0f %-12zu %-12lld\n",
           "add",
           n,
           ops_sec,
           (double)hashmap_bench_add_probes / (double)n,
           stats.avg_ns,
           hashmap_bench_resizes,
           rss);

    free(samples);
    free_hashmap(&map);
}

static void bench_get(size_t n, char **keys) {
    HashMap map = create_hashmap();
    for (size_t i = 0; i < n; i++) {
        add_hashmap(&map, keys[i], (int)i);
    }

    size_t sample_every = choose_sample_every(n);
    size_t sample_count = (n + sample_every - 1) / sample_every;
    long long *samples = malloc(sizeof(long long) * sample_count);
    size_t sample_index = 0;

    hashmap_bench_reset();
    long long start = now_ns();
    for (size_t i = 0; i < n; i++) {
        long long t0 = 0;
        if ((i % sample_every) == 0) {
            t0 = now_ns();
        }

        (void)get_hashmap(&map, keys[i]);

        if ((i % sample_every) == 0) {
            samples[sample_index++] = now_ns() - t0;
        }
    }
    long long elapsed = now_ns() - start;

    LatencyStats stats = compute_latency(samples, sample_index);
    double secs = (double)elapsed / 1000000000.0;
    double ops_sec = (double)n / secs;
    long long rss = rss_kb();

    printf("%-10s %-12zu %-15.0f %-10.2f %-15.0f %-12s %-12lld\n",
           "get",
           n,
           ops_sec,
           (double)hashmap_bench_get_probes / (double)n,
           stats.avg_ns,
           "-",
           rss);

    free(samples);
    free_hashmap(&map);
}

static void bench_pop(size_t n, char **keys) {
    HashMap map = create_hashmap();
    for (size_t i = 0; i < n; i++) {
        add_hashmap(&map, keys[i], (int)i);
    }

    size_t sample_every = choose_sample_every(n);
    size_t sample_count = (n + sample_every - 1) / sample_every;
    long long *samples = malloc(sizeof(long long) * sample_count);
    size_t sample_index = 0;

    hashmap_bench_reset();
    long long start = now_ns();
    for (size_t i = 0; i < n; i++) {
        long long t0 = 0;
        if ((i % sample_every) == 0) {
            t0 = now_ns();
        }

        (void)pop_hashmap(&map, keys[i]);

        if ((i % sample_every) == 0) {
            samples[sample_index++] = now_ns() - t0;
        }
    }
    long long elapsed = now_ns() - start;

    LatencyStats stats = compute_latency(samples, sample_index);
    double secs = (double)elapsed / 1000000000.0;
    double ops_sec = (double)n / secs;
    long long rss = rss_kb();

    printf("%-10s %-12zu %-15.0f %-10.2f %-15.0f %-12s %-12lld\n",
           "pop",
           n,
           ops_sec,
           (double)hashmap_bench_pop_probes / (double)n,
           stats.avg_ns,
           "-",
           rss);

    free(samples);
    free_hashmap(&map);
}

int main(void) {
#ifdef BENCH_QUICK
    const size_t sizes[] = {100, 1000, 10000};
#else
    const size_t sizes[] = {1000, 10000, 50000, 100000, 1000000, 10000000};
#endif
    const size_t sizes_count = sizeof(sizes) / sizeof(sizes[0]);

    print_header();
    bench_hash_cost("benchmark_key", 1000000);
    printf("\n");

    for (size_t i = 0; i < sizes_count; i++) {
        size_t n = sizes[i];
        char **keys = build_keys(n);
        if (!keys) {
            printf("Failed to allocate keys for n=%zu\n", n);
            return 1;
        }

        bench_add(n, keys);
        bench_get(n, keys);
        bench_pop(n, keys);

        free_keys(keys, n);
    }

    return 0;
}
