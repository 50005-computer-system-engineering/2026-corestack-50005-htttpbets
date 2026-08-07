#include "dict.h"

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
        return -1;
    }
    
    // check for a matching string
    for (int i=0; i<dict.n_items; i++)
    {
        if (!strcmp(dict.vals[i][0], key))
        {
            return i;
        }
    }
    
    // loop conclude if not found, catch and return as error
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
        return NULL;
    }

    // return appropriate pointer
    return dict.vals[i][1];
}
