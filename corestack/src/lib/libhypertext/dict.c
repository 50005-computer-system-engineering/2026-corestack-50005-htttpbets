#include "dict.h"
#include "utils/logger.h"

/*
searches Dictionary for index of a specific key
returns the valid index if found
returns -1 if not found or dict is not valid for search
*/
int get_dict_index(Dictionary dict, unsigned char *key)
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
unsigned char *get_dict_val(Dictionary dict, unsigned char *key)
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
