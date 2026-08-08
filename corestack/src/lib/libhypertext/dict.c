#include "dict.h"
#include "utils/logger.h"
#include <stdlib.h>

/*
Allocates appropriate memory for the new dict
Returns pointer on success, NULL on failure
*/
struct Dictionary *dict_init(int size)
{
    struct Dictionary *new_dict = malloc(sizeof(struct Dictionary) + size * sizeof(unsigned char *));

    if (new_dict == NULL)
    {
        perror("[dict_init()] malloc");
        LOG_E("[dict_init()] could not allocate memory for Dictionary");
        return NULL;
    }

    LOG_I("[dict_init()] created dict with size %d", size);
    new_dict->n_items = size;
    return new_dict;
}

/*
searches Dictionary for index of a specific key
returns the valid index if found
returns -1 if not found or dict is not valid for search
*/
int get_dict_index(struct Dictionary dict, unsigned char *key)
{
    // check if dict is valid size
    if (dict.n_items <= 0)
    {
        LOG_E("[get_dict_index()] Dictionary is too small or does not exist");
        return -1;
    }
    
    // check for a matching string
    for (int i=0; i<dict.n_items; i++)
    {
        if (!strcmp(dict.vals[i][0], key))
        {
            LOG_D("[get_dict_index()] found key %s at index %d", key, i);
            return i;
        }
    }
    
    // loop conclude if not found, catch and return as error
    LOG_E("[get_dict_index()] key %s was not found in Dictionary", key);
    return -1;
}

/*
returns the val string of specific key string in Dictionary if it exists
returns NULL in case where key is not found
*/
unsigned char *get_dict_val(struct Dictionary dict, unsigned char *key)
{
    // get index to see if key exists
    int i = get_dict_index(dict, key);
    if (i < 0)
    {
        LOG_E("[get_dict_val()] could not get index of key %s in Dictionary", key);    
        return NULL;
    }

    // return appropriate pointer
    unsigned char *val = dict.vals[i][1];
    LOG_D("[get_dict_val()] found key-value pair {%s : %s}", key, val);
    return val;
}

/*
adds a key-value pair to dict at a specified index
overwrites previous value
returns 0 on success, 1 on failure
*/
int add_key_value_pair(struct Dictionary *dict, unsigned char *key, unsigned char *val, int idx)
{
    // check if input
    if (dict == NULL)
    {
        LOG_E("[add_key_value_pair()] no existing dictionary");
        return -1;
    }
    if (idx >= dict->n_items)
    {
        LOG_E("[add_key_value_pair()] index out of range");
        return -1;
    }
    
    // assign direct to the index
    dict->vals[idx][0] = key;
    dict->vals[idx][1] = val;
    LOG_I("[add_key_value_pair()] added key-value pair {%s : %s} at index %d", key, val, idx);
    return 0;
}