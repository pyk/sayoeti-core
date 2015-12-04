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

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

 #include "dict.h"
 #include "utils.h"

/* dict_item_new: creates new dictionary item with term TERM */
struct dict_item *dict_item_new(char *term)
{
    struct dict_item *item = (struct dict_item *)malloc(sizeof(struct dict_item));
    if(item == NULL) {
        return NULL;
    }

    /* Copy term to new memory; to avoid the term removed from the outside of 
     * dict_item_destroy */
    char *t = (char *)malloc(sizeof(char) * (strlen(term) + 1));
    if(t == NULL) {
        return NULL;
    }

    /* Copy TERM to T */
    strcpy(t, term);

    item->index = 0;
    item->term = t;
    item->is_inserted = FALSE;
    item->ndocs = 0;
    item->height = 1;
    item->left = NULL;
    item->right = NULL;

    return item;
}

/* dict_item_destroy: recursively free all allocated memory for the dictionary
 * items; started from dictionary items ROOT */
void dict_item_destroy(struct dict_item *root) 
{
    if(root == NULL) {
        return;
    } 

    dict_item_destroy(root->left);
    dict_item_destroy(root->right);

    free(root->term);
    free(root);
}

/* dict_item_height: get the height of dictionary item ITEM */
int dict_item_height(struct dict_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return item->height;
}

/* dict_item_get_balance: get the balance factor of dictionary item ITEM */
int dict_item_get_balance(struct dict_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return dict_item_height(item->left) - dict_item_height(item->right);
}

/* dict_item_rotate_right: Performs AVL tree right rotation on 
 * dictionary item ITEM. It returns right rotated dicitonary item. */
struct dict_item *dict_item_rotate_right(struct dict_item *item)
{
    struct dict_item *backup_left = item->left;
    struct dict_item *backup_right = backup_left->right;

    /* Perform rotation */
    backup_left->right = item;
    item->left = backup_right;

    /* Update the heights */
    int hil = dict_item_height(item->left);
    int hir = dict_item_height(item->right);
    item->height = util_max(hil, hir) + 1;

    int bll = dict_item_height(backup_left->left);
    int blr = dict_item_height(backup_left->right);
    backup_left->height = util_max(bll, blr) + 1;

    /* Return rotated item */
    return backup_left;
}

/* dict_item_rotate_right: Performs AVL tree left rotation on 
 * dictionary item ITEM. It returns left rotated dicitonary item. */
struct dict_item *dict_item_rotate_left(struct dict_item *item)
{
    struct dict_item *backup_right = item->right;
    struct dict_item *backup_left = backup_right->left;

    /* Perform rotation */
    backup_right->left = item;
    item->right = backup_left;

    /* Update the heights */
    int hil = dict_item_height(item->left);
    int hir = dict_item_height(item->right);
    item->height = util_max(hil, hir) + 1;

    int brl = dict_item_height(backup_right->left);
    int brr = dict_item_height(backup_right->right);
    backup_right->height = util_max(brl, brr) + 1;

    /* Return rotated item */
    return backup_right;
}

/* dict_item_insert: Inserts dictionary item ITEM to a dictionary items root ROOT. 
 * If the root is NULL then ITEM will be the root of the dictionary */
struct dict_item *dict_item_insert(struct dict_item *root, struct dict_item *item) 
{
    /* Set root for the first time or assign to root->left or root->right */
    if(root == NULL) {
        item->is_inserted = TRUE;
        return item;
    }

    /* We compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(root->term, item->term) > 0) {
        root->left = dict_item_insert(root->left, item);
    } else if(strcmp(root->term, item->term) < 0) {
        root->right = dict_item_insert(root->right, item);
    }

    /* Update the height of the ancestor node */
    int hrl = dict_item_height(root->left);
    int hrr = dict_item_height(root->right);
    root->height = util_max(hrl, hrr) + 1;
    
    /* Get the balance factor to check wether tree is balance or unbalanced */
    int balance = dict_item_get_balance(root);

    /* 4 case if the tree is unbalanced 
     * Case 1: Left left case */
    if(balance > 1 && (strcmp(root->left->term, item->term) > 0)) {
        /* Perform right rotation */
        return dict_item_rotate_right(root);
    }

    /* Case 2: Right right case */
    if(balance < -1 && (strcmp(root->right->term, item->term) < 0)) {
        /* Perform left rotation */
        return dict_item_rotate_left(root);
    }

    /* Case 3: Left right case */
    if(balance > 1 && (strcmp(root->left->term, item->term) < 0)) {
        root->left = dict_item_rotate_left(root->left);
        return dict_item_rotate_right(root);
    }

    /* Case 4: Right left case */
    if(balance < -1 && (strcmp(root->right->term, item->term) > 0)) {
        root->right = dict_item_rotate_right(root->right);
        return dict_item_rotate_left(root);
    }

    return root;
}

/* dict_item_exists: recursively search the dictionary item ITEM from
 * root dictionary ROOT. It returns TRUE if the item is found, otherwise 
 * FALSE */
int dict_item_exists(struct dict_item *root, struct dict_item *item)
{
    if(root == NULL) {
        return FALSE;
    }

    /* we compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(root->term, item->term) == 0) {
        return TRUE;
    } else if(strcmp(root->term, item->term) < 0) {
        return dict_item_exists(root->right, item);
    } else if(strcmp(root->term, item->term) > 0) {
        return dict_item_exists(root->left, item);
    }

    return FALSE;
}

/* dict_item_search: recursively search the token TOKEN in root dictionary 
 * ROOT. It returns NULL if the token is not exists in dictionary */
struct dict_item *dict_item_search(struct dict_item *root, char *term)
{
    /* We can't find the term in the root dictionary ROOT */
    if(root == NULL) {
        return NULL;
    }

    /* We compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(root->term, term) == 0) {
        return root;
    } else if(strcmp(root->term, term) > 0) {
        return dict_item_search(root->left, term);
    } else if(strcmp(root->term, term) < 0) {
        return dict_item_search(root->right, term);
    }

    return NULL;
}

/* dict_item_print: recursivly print each item in the dictionary 
 * in alphabetical order. */
void dict_item_print(struct dict_item *root)
{
    if(root->left) dict_item_print(root->left);
    printf("%li:%d:%s\n", root->index, root->ndocs, root->term);
    if(root->right) dict_item_print(root->right);
}

/* dict_new: Initialize the dictionary from source SOURCE. The dictionary 
 * source can be path to a file or path to a dictionary that relative to
 * where the program is executed. Usually it's corpus directory or stop
 * words file. */
struct dict *dict_new(char *source)
{
    /* Allocate memory for dictionary */
    struct dict *d = (struct dict *)malloc(sizeof(struct dict));
    if(d == NULL) {
        return NULL;
    }

    /* Copy *source to new memory; to avoid the *srouce removed from 
     * the outside of dict_destroy */
    char *s = (char *)malloc(sizeof(char) * (strlen(source) + 1));
    if(s == NULL) {
        return NULL;
    }

    /* SOURCE to S */
    strcpy(s, source);

    d->source = s;
    d->ndocs = 0;
    d->nitems = 0;
    d->root = NULL;
    return d;
}

/* dict_destroy: remove dictionary D from memory */
void dict_destroy(struct dict *d)
{   
    /* Remove all items from dictionary */
    dict_item_destroy(d->root);
    free(d->source);
    free(d);
}

/* Print all items in dictionary DICT */
void dict_printout(struct dict *d)
{
    printf("DICTIONARY %li documents %li token; \"%s\" as a root with height %d\n", 
        d->ndocs, d->nitems, d->root->term, d->root->height);
    dict_item_print(d->root);
}

/* dict_populatef: Populates dictionary D items from file FP.
 * The item is inserted if not exists in SW. If EXC is NULL then 
 * exists checking is omitted.
 * 
 * It returns populated dictionary */
struct dict *dict_populatef(FILE *fp, struct dict *exc, struct dict *d)
{
    /* Read every token in the file FP and populate the dictionary D
     * exlude all files in dictioanary EXC */
    char token[MAX_TOKEN_CHAR];
    int lentoken;
    while((lentoken = util_tokenf(token, MAX_TOKEN_CHAR, fp)) != 0) {
        /* The token length is exceeded, so skip the token we get the next 
         * token instead */
        if(lentoken >= MAX_TOKEN_CHAR-1) {
            continue;
        }

        /* Create new dictionary item with term TOKEN */
        struct dict_item *vocab = dict_item_new(token);
        if(vocab == NULL) {
            return NULL;
        }
        
        /* Check wether the words is in EXC (excluded) directory
         * or not. */
        int exists = FALSE;
        if(exc) {
            exists = dict_item_exists(exc->root, vocab);
        }

        /* Insert the dictionary item if not exists */
        if(!exists) {
            /* Insert dictionary item VOCAB to a dictionary root D */
            d->root = dict_item_insert(d->root, vocab);
            /* Keep track of newly inserted items */
            if(vocab->is_inserted) {
                /* increment nitems, update vocab */
                d->nitems += 1;
                vocab->index = d->nitems;
            }
        }

        /* Remove if the item is exists in dictionary EXC */
        if(exists) {
            dict_item_destroy(vocab);
        }
        
    }

    /* Return populated dictionary */
    return d;
}