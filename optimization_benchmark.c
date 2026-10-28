#include "optimization_support.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct BenchRecord { int key; } BenchRecord;
static int compare_records(const void *left, const void *right)
{
    const BenchRecord *a = (const BenchRecord *)left;
    const BenchRecord *b = (const BenchRecord *)right;
    return (a->key > b->key) - (a->key < b->key);
}
static double seconds_between(clock_t start, clock_t end)
{
    return (double)(end - start) / (double)CLOCKS_PER_SEC;
}
int main(void)
{
    const size_t count = 10000U;
    BenchRecord *records = (BenchRecord *)malloc(count * sizeof(*records));
    void **pointers = (void **)malloc(count * sizeof(*pointers));
    RailwayPointerIndex index = {NULL, 0U, 0U};
    BenchRecord key;
    clock_t start, end;
    size_t i;
    volatile size_t sequential_hits = 0U;
    volatile size_t binary_hits = 0U;
    if (records == NULL || pointers == NULL) { free(records); free(pointers); return 1; }
    for (i = 0U; i < count; ++i) { records[i].key = (int)(count - i); pointers[i] = &records[i]; }
    if (railway_pointer_index_build(&index, pointers, count, compare_records) != 0) { free(records); free(pointers); return 1; }
    key.key = (int)(count / 2U);
    start = clock();
    for (i = 0U; i < count; ++i) {
        size_t j;
        for (j = 0U; j < count; ++j) {
            if (records[j].key == key.key) { ++sequential_hits; break; }
        }
    }
    end = clock();
    printf("Sequential search (%zu lookups): %.6f seconds (%zu hits)\n", count, seconds_between(start, end), (size_t)sequential_hits);
    start = clock();
    for (i = 0U; i < count; ++i) if (railway_pointer_index_find(&index, &key, compare_records) != NULL) ++binary_hits;
    end = clock();
    printf("Sorted-index binary search (%zu lookups): %.6f seconds (%zu hits)\n", count, seconds_between(start, end), (size_t)binary_hits);
    railway_pointer_index_destroy(&index);
    free(records);
    free(pointers);
    return 0;
}
