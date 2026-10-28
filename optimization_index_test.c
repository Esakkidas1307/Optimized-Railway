#include "optimization_support.h"
#include <stdio.h>
#include <string.h>

typedef struct TestRecord {
    int key;
    const char *name;
} TestRecord;

static int compare_records(const void *left, const void *right)
{
    const TestRecord *a = (const TestRecord *)left;
    const TestRecord *b = (const TestRecord *)right;
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

int main(void)
{
    TestRecord a = {12628, "A"};
    TestRecord b = {12615, "B"};
    TestRecord c = {12627, "C"};
    TestRecord key = {12627, ""};
    void *items[] = {&a, &b, &c};
    RailwayPointerIndex index = {NULL, 0U, 0U};
    int ok = 1;

    if (railway_pointer_index_build(&index, items, 3U, compare_records) != 0) {
        fprintf(stderr, "index build failed\n");
        return 1;
    }
    if (((TestRecord *)index.items[0])->key != 12615 ||
        ((TestRecord *)index.items[1])->key != 12627 ||
        ((TestRecord *)index.items[2])->key != 12628) {
        fprintf(stderr, "index sort order failed\n");
        ok = 0;
    }
    if (railway_pointer_index_find(&index, &key, compare_records) != &c) {
        fprintf(stderr, "binary search failed\n");
        ok = 0;
    }
    key.key = 99999;
    if (railway_pointer_index_find(&index, &key, compare_records) != NULL) {
        fprintf(stderr, "missing-key lookup failed\n");
        ok = 0;
    }
    railway_pointer_index_destroy(&index);
    if (ok != 0) puts("optimization index tests passed");
    return ok != 0 ? 0 : 1;
}
