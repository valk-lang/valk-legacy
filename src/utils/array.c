
#include "../all.h"

Array *array_make(Allocator *alc, int max_length) {
    Array *arr = al(alc, sizeof(Array));
    arr->alc = alc;
    arr->length = 0;
    arr->max_length = max_length;
    arr->data = al(alc, sizeof(void *) * max_length);
    return arr;
}

void array_push(Array *arr, void *item) {
    if (arr->length == arr->max_length) {
        int newlen = arr->max_length * 2;
        void *new = al(arr->alc, newlen * sizeof(void *));
        memcpy(new, arr->data, arr->max_length * sizeof(void *));
        arr->data = new;
        arr->max_length = newlen;
    }
    uintptr_t *adr = arr->data + (arr->length * sizeof(void *));
    *adr = (uintptr_t)item;
    arr->length++;
}
void array_push_u32(Array *arr, unsigned int value) {
    array_push(arr, (void*)(uintptr_t)value);
}

void array_push_unique_adr(Array *arr, void *item) {
    if (!array_contains(arr, item, arr_find_adr)) {
        array_push(arr, item);
    }
}
void array_push_unique_chars(Array *arr, char *item) {
    if (!array_contains(arr, item, arr_find_str)) {
        array_push(arr, item);
    }
}

void *array_pop(Array *arr) {
    if (arr->length == 0) {
        return NULL;
    }
    arr->length--;
    uintptr_t *adr = arr->data + ((arr->length) * sizeof(void *));
    return (void *)(*adr);
}

void array_shift(Array *arr, void *item) {
    if (arr->length == arr->max_length) {
        int newlen = arr->max_length * 2;
        void *new = al(arr->alc, newlen * sizeof(void *));
        memcpy(new, arr->data, arr->max_length * sizeof(void *));
        // free(arr->data);
        arr->data = new;
        arr->max_length = newlen;
    }
    int i = arr->length;
    void *data = arr->data;
    while (i > 0) {
        void **from = data + (i - 1) * sizeof(void *);
        void **to = data + i * sizeof(void *);
        *to = *from;
        i--;
    }
    arr->length++;
    void **adr = arr->data;
    *adr = item;
}

void *array_pop_first(Array *arr) {
    if (arr->length == 0) {
        return NULL;
    }
    void *data = arr->data;
    void *first = *(void **)data;

    int last_i = arr->length - 1;
    int i = 0;
    while (i < last_i) {
        void **from = data + (i + 1) * sizeof(void *);
        void **to = data + i * sizeof(void *);
        *to = *from;
        i++;
    }

    arr->length--;
    return first;
}

void *array_get_index(Array *arr, int index) {
    if (index >= arr->length) {
        return NULL;
    }
    uintptr_t *result = arr->data + (index * sizeof(void *));
    return (void *)*result;
}
unsigned int array_get_index_u32(Array *arr, int index) {
    return (unsigned int)(intptr_t)array_get_index(arr, index);
}

void array_set_index(Array *arr, int index, void *item) {
    if (index > arr->max_length - 1) {
        printf("Array set index out of range (compiler bug)\n");
        raise(11);
    }
    if ((index + 1) > arr->length) {
        arr->length = index + 1;
    }
    uintptr_t *adr = arr->data + (index * sizeof(void *));
    *adr = (uintptr_t)item;
}

bool array_contains(Array *arr, void *item, int type) {
    int index = array_find(arr, item, type);
    return index > -1;
}
bool array_contains_i32(Array *arr, int value) {
    return array_find(arr, (void*)(uintptr_t)value, arr_find_int) > -1;
}
bool array_contains_u32(Array *arr, unsigned int value) {
    return array_find(arr, (void*)(uintptr_t)value, arr_find_int) > -1;
}

int array_find(Array *arr, void *item, int type) {
    int x = arr->length;
    while (x > 0) {
        x--;
        uintptr_t *adr = arr->data + (x * sizeof(void *));
        if (type == arr_find_adr) {
            if (*adr == (uintptr_t)item)
                return x;
        } else if (type == arr_find_str) {
            char *a = (char *)*adr;
            char *b = (char *)item;
            if (str_is(a, b))
                return x;
        } else if (type == arr_find_int) {
            if ((int)(*adr) == *(int *)item)
                return x;
        } else {
            die("array.c invalid search type");
        }
    }
    return -1;
}
int array_find_x(Array *arr, void *item, int type, int start, int end) {
    int x = start;
    while (x < end) {
        uintptr_t *adr = arr->data + (x * sizeof(void *));
        if (type == arr_find_adr) {
            if (*adr == (uintptr_t)item)
                return x;
        } else if (type == arr_find_str) {
            char *a = (char *)*adr;
            char *b = (char *)item;
            if (str_is(a, b))
                return x;
        } else if (type == arr_find_int) {
            if ((int)(*adr) == *(int *)item)
                return x;
        } else {
            die("array.c invalid search type");
        }
        x++;
    }
    return -1;
}

Array *array_merge(Allocator *alc, Array *arr1, Array *arr2) {
    Array* result = array_make(alc, 2);
    if (arr1) {
        loop(arr1, i) {
            array_push(result, array_get_index(arr1, i));
        }
    }
    if (arr2) {
        loop(arr2, i) {
            array_push(result, array_get_index(arr2, i));
        }
    }
    return result;
}
