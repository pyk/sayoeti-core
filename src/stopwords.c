/* Sayoeti Stop Words (SW)
 * Collection of common function to create stop words dictionary.
 *
 * Copyright 2015 Bayu Aldi Yansyah <bayualdiyansyah@gmail.com>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>

#include "stopwords.h"
#include "dict.h"
 
/* stopw_dict_create: creates stop words dictionary from file FNAME. 
 * Returns NULL if only if error happen and ERRNO will be set to last
 * error. */
struct dict *stopw_dict_create(char *fname)
{
    /* Initialize the dictionary */
    struct dict *stopw_dict = dict_new(fname);
    if(stopw_dict == NULL) {
        return NULL;
    }

    /* Read the file fname */
    FILE *fp = fopen(fname, "r");
    if(fp == NULL) {
        return NULL;
    }

    /* Populate dictionary from a file FP */
    stopw_dict = dict_populatef(fp, NULL, stopw_dict);
    if(stopw_dict == NULL) {
        return NULL;
    }

    /* Count document as 1 */
    stopw_dict->ndocs = 1;

    /* Close the file */
    if(fclose(fp) != 0) {
        return NULL;
    }

    return stopw_dict;
}
