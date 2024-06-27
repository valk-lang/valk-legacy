
#ifndef H_ARR
#define H_ARR

#include "typedefs.h"

enum ARRFINDTYPE {
    arr_find_adr,
    arr_find_str,
    arr_find_int,
};

struct Array {
    Allocator *alc;
    int length;
    int max_length;
    void *data;
};

Array *array_make(Allocator *alc, int max_length);
void array_push(Array *, void *);
void array_push_unique_adr(Array *arr, void *item);
void array_push_unique_chars(Array *arr, char *item);
void *array_pop(Array *arr);
void *array_pop_first(Array *arr);
bool array_contains(Array *, void *, int);
int array_find(Array *, void *, int);
int array_find_x(Array *arr, void *item, int type, int start, int end);
void array_shift(Array *arr, void *item);
void *array_get_index(Array *, int);
void array_set_index(Array *, int, void *);
Array *array_merge(Allocator *alc, Array *arr1, Array *arr2);

// Integer values
void array_push_u32(Array *arr, unsigned int value);
bool array_contains_i32(Array *arr, int value);
bool array_contains_u32(Array *arr, unsigned int value);
unsigned int array_get_index_u32(Array *arr, int index);

#endif
