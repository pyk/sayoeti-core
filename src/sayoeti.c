/* Sayoeti CLI (command line interface)
 *
 * Copyright (c) 2015, Bayu Aldi Yansyah <bayualdiyansyah@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <string.h>

/****************************
 * Dictionary
 * We use AVL binary search tree here. for fastest search.
 ****************************/
#define TRUE 1
#define FALSE 0

/* This is the maximum number of characters in one token.
 * see: https://en.wikipedia.org/wiki/Longest_Words#Indonesian */
#define MAX_TOKEN_CHAR 31 

/* dict_item represents a node of AVL Binary Search Tree (BST) */
struct dict_item {
    /* basic info for each item in dictionary; unique index and string term */
    long index;
    char *term;

    /* 0 if not, otherwise 1 */
    int is_inserted; 

    /* The height property for AVL tree */
    int height;
    struct dict_item *left, *right;
};

/* dict_item_new: Creates new dictionary item with term TERM */
struct dict_item *dict_item_new(char *term)
{
    /* TODO(pyk): Handle if memory allocation fail */
    struct dict_item *item = (struct dict_item *)malloc(sizeof(struct dict_item));
    /* Just print the message for now */
    if(item == NULL) {
        fprintf(stderr, "error in dict_item_new: couldn't allocate the memory for dictionary item.\n");
        return NULL;
    }

    item->index = 0;
    item->term = term;
    item->is_inserted = FALSE;
    item->height = 1;
    item->left = NULL;
    item->right = NULL;

    return item;
}

/* dict_item_height_max: returns the maximum value of H1 and H2 */
int dict_item_height_max(int h1, int h2) {
    return (h1 > h2) ? h1 : h2;
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

/* dict_item_rotate_right: Performs tree operation right rotation on 
 * dictionary item ITEM. It returns rotated dicitonary item. */
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
    item->height = dict_item_height_max(hil, hir) + 1;

    int bll = dict_item_height(backup_left->left);
    int blr = dict_item_height(backup_left->right);
    backup_left->height = dict_item_height_max(bll, blr) + 1;

    /* Return rotated item */
    return backup_left;
}

/* dict_item_rotate_left: Performs tree operation left rotation on 
 * dictionary item ITEM. It returns rotated dicitonary item. */
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
    item->height = dict_item_height_max(hil, hir) + 1;

    int brl = dict_item_height(backup_right->left);
    int brr = dict_item_height(backup_right->right);
    backup_right->height = dict_item_height_max(brl, brr) + 1;

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
    root->height = dict_item_height_max(hrl, hrr) + 1;
    
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

/* dict_item_print: recursivly print each item in the dictionary 
 * in alphabetical order. */
void dict_item_print(struct dict_item *root)
{
    if(root->left) dict_item_print(root->left);
    printf("%li:%s\n", root->index, root->term);
    if(root->right) dict_item_print(root->right);
}

/* dict_item_destroy: recursively remove the dictionary items from memory 
 * started at dictionary items ROOT */
void dict_item_destroy(struct dict_item *root) 
{
    if(root == NULL) {
        return;
    } 

    dict_item_destroy(root->left);
    dict_item_destroy(root->right);

    free(root);
}

/* dict: represents the dictionary */
struct dict {
    /* Source of dictionary */
    char *source;
    /* Keep track how many items in the dictionary */
    long nitems;
    /* the root of dictionary */
    struct dict_item *root;
};

/* dict_init: Initialize the dictionary from source SOURCE. The dictionary 
 * source can be path to a file or path to a dictionary that relative to
 * where the program is executed. */
struct dict *dict_init(char *source)
{
    struct dict *d = (struct dict *)malloc(sizeof(struct dict));
    d->source = source;
    d->nitems = 0;
    d->root = NULL;
    return d;
}

/* Print all items in dictionary DICT */
void dict_printout(struct dict *d)
{
    printf("DICT: nitems: %li root: %s height: %d\n", d->nitems, d->root->term, d->root->height);
    dict_item_print(d->root);
}

/* dict_destroy: remove dictionary D from memory */
void dict_destroy(struct dict *d) 
{ 
    dict_item_destroy(d->root);
    free(d);
}

/* dic_populate_from_file: Populates dictionary D items from file FP.
 * It returns new */
struct dict *dict_populate_from_file(struct dict *d, FILE *fp)
{
    /* To save a discovered token temporary */
    char token[MAX_TOKEN_CHAR];
    int ti = 0;

    /* Read every char C in the file FP until end of file */
    int c;
    while((c = fgetc(fp)) != EOF) {
        /* We only care if the char is alphabet */
        if(isalpha(c)) {
            /* Save the char C to token, convert all char to lowercase */
            if (ti < (MAX_TOKEN_CHAR - 2)) {
                token[ti] = tolower(c);
            }

            /* Increase the token index */
            ti++;
        } else if(isspace(c)) {
            /* If the next char is a whitespace, then close the previous 
             * discovered token then create a new dictionary item and insert
             * it to a dictionary D */ 

             /* Make sure we don't access overflow buffer */
            if(ti != 0 && ti < (MAX_TOKEN_CHAR-2)) {
                /* Terminate the current token */
                token[ti] = '\0';

                /* Save the token to a t variable, to avoid re-used token pointer
                 * across a dictionary items. */
                char *t = (char *)malloc(sizeof(char) * (strlen(token) + 1));
                /* If we cannot allocate the memory, just print the message */
                if(t == NULL) {
                    /* TODO(pyk): remove this error printout */
                    fprintf(stderr, "error in dict_populate_from_file: couldn't allocate the memory for token.\n");
                    return NULL;
                }
                /* Copy TOKEN to T */
                strcpy(t, token);

                /* Create new dictionary item with term T */
                struct dict_item *vocab = dict_item_new(t);
                if(vocab == NULL) {
                    /* TODO(pyk): remove this error printout */
                    fprintf(stderr, "error in dict_populate_from_file: couldn't create new dictionary item.\n");
                    return NULL;
                }
                
                /* Insert dictionary item VOCAB to a dictionary root D */
                d->root = dict_item_insert(d->root, vocab);
                /* Keep track of newly inserted items */
                if(vocab->is_inserted) {
                    /* increment nitems, update vocab */
                    d->nitems += 1;
                    vocab->index = d->nitems;
                }
                
                /* All allocated memory in here will be freed when dictionary
                 * DICT is destroyed */
            }

            /* Reset the token index */
            ti = 0;
        }
    }

    /* Return populated dictionary */
    return d;
}

/****************************
 * Arguments program parser
 * This started from main in argp_parse function.
 ****************************/
const char *argp_program_version = "0.0.1";
const char *argp_program_bug_address = "bayualdiyansyah@gmail.com";
const char *short_desc = "Sayoeti -- An AI that can understand which document is about Indonesian corruption news";

/* Available options for the program; used by argp_parser */
const struct argp_option available_options[] = {
    {"corpus", 'c', "DIR", 0, "Path to corpus directory (required)" },
    {"stopwords", 's', "FILE", 0, "File containing new line separated stop words (optional)" },
    {"listen", 'l', "PORT", 0, "Port to listen too (optional)" },
    { 0 } // entry for termination
};

/* options structure used as input by argp_parse; it will passed to argp
 * parser function parse_opt as state->input. */
struct options {
    char *corpus_dir;
    char *stopwords_file;
    char *port;
};

/* parse_opt get called for each option parsed; used by arg_parser */
error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /* get the input */
    struct options *opts = state->input;
    switch (key) {
    case 'c':
        opts->corpus_dir = arg;
        break;
    case 's':
        opts->stopwords_file = arg;
        break;
    case 'l':
        opts->port = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/****************************
 * Stop words
 ****************************/
/* stopw_error: provide convinient way to report & handle an error */
struct stopw_err {
    char *msg; /* error message */
};

/* stopw_dict_create: creates stopwords dictionary D from file FNAME. 
 * Returns NULL if only if error happen and ERRNO will be set to last
 * error. */
struct dict *stopw_dict_create(char *fname, struct dict *d)
{
    /* Initialize the dictionary */
    d = dict_init(fname);

    /* Read the file fname */
    FILE *fp = fopen(fname, "r");
    if(fp == NULL) {
        return NULL;
    }

    /* Populate dictionary from a file FP */
    d = dict_populate_from_file(d, fp);
    if(d == NULL) {
        return NULL;
    }

    /* Close the file; only display info if error */
    if(fclose(fp) != 0) {
        return NULL;
    }

    return d;
}

/****************************
 * Main program
 ****************************/
int main(int argc, char** argv) {

    /* Set default value for each available option */
    struct options opts;
    opts.corpus_dir = NULL;
    opts.stopwords_file = NULL;
    opts.port = NULL;

    /* Parse the arguments; every option seen by parse_opt 
     * will be reflected in opts. */
    struct argp argp_parser = {available_options, parse_opt, 0, short_desc};
    argp_parse(&argp_parser, argc, argv, 0, 0, &opts);

    /* Exit if corpus_dir is not specified */
    if(!opts.corpus_dir) {
        fprintf(stderr, "-c options is required. Please see %s --help\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Skip building stop words dictionary if the FILE is not provided */
    if(!opts.stopwords_file) {
        printf("sayoeti: Stop words file is not specified.\n");
        printf("sayoeti: Skipping process building stop words dictionary.\n");
    }

    /* Create stop words dictionary if the FILE is specified */
    struct dict *stopwords_dict = NULL;
    if(opts.stopwords_file) {
        printf("sayoeti: Create stop words dictionary from %s\n", opts.stopwords_file);
        stopwords_dict = stopw_dict_create(opts.stopwords_file, stopwords_dict);
        if(stopwords_dict == NULL) {
            fprintf(stderr, "sayoeti: Couldn't create dicitonary from file: %s; %s\n", opts.stopwords_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("sayoeti: stop words dictionary from %s is created.\n", opts.stopwords_file);
    }

    if(stopwords_dict) {
        struct dict_item *vocab = dict_item_new("tidass");
        if(dict_item_exists(stopwords_dict->root, vocab)) {
            printf("%s\n", "EXISTS");
        } else {
            printf("%s\n", "NOT EXISTS");
        }
        
        // dict_printout(stopwords_dict);
    }


    printf("CORPUS = %s\n", opts.corpus_dir);
    printf("STOPWORDS = %s\n", opts.stopwords_file);
    printf("PORTS = %s\n", opts.port);

    dict_destroy(stopwords_dict);
    return 0;
}