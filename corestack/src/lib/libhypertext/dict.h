#ifndef LIBHT_DICT_H
#define LIBHT_DICT_H

#include <string.h>

/*
Created this as a dictionary for storage of headers and body
*/
struct Dictionary {
    int n_items;
    unsigned char *vals[][2];
};

struct Dictionary *dict_init(int size);
int get_dict_index(struct Dictionary dict, unsigned char *key);
unsigned char *get_dict_val(struct Dictionary dict, unsigned char *key);
int add_key_value_pair(struct Dictionary *dict, unsigned char *key, unsigned char *val, int idx)

#endif