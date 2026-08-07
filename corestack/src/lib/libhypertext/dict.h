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

int get_dict_index(struct Dictionary dict, unsigned char *key);
unsigned char *get_dict_val(struct Dictionary dict, unsigned char *key);

#endif