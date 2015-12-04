/* Sayoeti Dictionary
 * This dictionary allow us create stopwords and index vocabulary
 * using imlementation of AVL Binary Search Tree (BST).
 * We use AVL BST because it's allow us to lookup the term in O(log n).
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

#ifndef DICT_H
#define DICT_H

/* Macros */
#define TRUE 1
#define FALSE 0
/* This is the maximum number of characters in one token.
 * see: https://en.wikipedia.org/wiki/Longest_Words#Indonesian */
#define MAX_TOKEN_CHAR 31

/* dict_item: represents a node of AVL Binary Search Tree (BST);
 * ii used as one word in dictionary */
struct dict_item {
    /* Basic info for each item in dictionary; 
     * unique index and string term */
    long index;
    char *term;

    /* 0 if not inserted, otherwise 1 */
    int is_inserted; 

    /* The height property for AVL tree */
    int height;

    /* Number of documents that contain this item;
     * This is for computing IDF */
    int ndocs;

    struct dict_item *left, *right;
};



/* dict: represents the dictionary */
struct dict {
    /* Source of dictionary */
    char *source;
    
    /* Keep track of how many source documents in the dictionary */
    long ndocs;

    /* Keep track how many items in the dictionary */
    long nitems;

    /* The root of dictionary */
    struct dict_item *root;
};


/* Prototypes */
struct dict_item *dict_item_new(char *term);
void dict_item_destroy(struct dict_item *root);
int dict_item_height_max(int h1, int h2);
int dict_item_height(struct dict_item *item);
int dict_item_get_balance(struct dict_item *item);
struct dict_item *dict_item_rotate_right(struct dict_item *item);
struct dict_item *dict_item_rotate_left(struct dict_item *item);
struct dict_item *dict_item_insert(struct dict_item *root, struct dict_item *item);
int dict_item_exists(struct dict_item *root, struct dict_item *item);
struct dict_item *dict_item_search(struct dict_item *root, char *term);
void dict_item_print(struct dict_item *root);

struct dict *dict_new(char *source);
void dict_destroy(struct dict *d);
void dict_printout(struct dict *d);
struct dict *dict_populatef(FILE *fp, struct dict *exc, struct dict *d);

#endif