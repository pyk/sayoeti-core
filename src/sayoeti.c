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

    /* The height property for AVL tree */
    int height;
    struct dict_item *parent, *left, *right;
};

/* dictionary represents the dictionary */
struct dictionary {
    /* track how many items in the dictionary */
    long nitems;

    /* the root of dictionary */
    struct dict_item *root;
};

/* Initialize the dictionary */
struct dictionary *dict_init(void)
{
    struct dictionary *dict = (struct dictionary *)malloc(sizeof(struct dictionary));
    dict->nitems = 0;
    dict->root = NULL;
    return dict;
}

/* Print each item in the dictionary in alphabetical order */
void dict_item_print(struct dict_item *item)
{
    if(item->left) dict_item_print(item->left);
    printf("%li:%s\n", item->index, item->term);
    if(item->right) dict_item_print(item->right);
}

/* Print all items in dictionary DICT */
void dict_printout(struct dictionary *dict)
{
    printf("DICT: nitems: %li root: %s height: %d\n", dict->nitems, dict->root->term, dict->root->height);
    dict_item_print(dict->root);
}

/* Destroy the dictionary item DICT */
void dict_item_destroy(struct dict_item *item) 
{
    if(item == NULL) {
        return;
    } 

    dict_item_destroy(item->left);
    dict_item_destroy(item->right);

    free(item->term);
    free(item);
}

/* Destroy dictionary DICT */
void dict_destroy(struct dictionary *dict) 
{ 
    dict_item_destroy(dict->root);

    free(dict);
}

/* Create new dictionary item with term TERM */
struct dict_item *dict_item_new(char *term)
{
    struct dict_item *item = (struct dict_item *)malloc(sizeof(struct dict_item));
    item->parent = NULL;
    item->left = NULL;
    item->right = NULL;
    item->index = 0;
    item->term = term;
    item->height = 1;

    return item;
}

/* Choose the maximum value */
int max(int a, int b) {
    return (a > b) ? a : b;
}

/* Return the height of item ITEM */
int dict_item_height(struct dict_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return item->height;
}

/* Get the balance factor of node ITEM */
int dict_item_get_balance(struct dict_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return dict_item_height(item->left) - dict_item_height(item->right);
}

/* Perform right rotation on node ITEM */
struct dict_item *dict_item_rotate_right(struct dict_item *item)
{
    struct dict_item *backup_left = item->left;
    struct dict_item *backup_right = backup_left->right;

    /* Perform rotation */
    backup_left->right = item;
    item->left = backup_right;

    /* Update the heights */
    item->height = max(dict_item_height(item->left), dict_item_height(item->right)) + 1;
    backup_left->height = max(dict_item_height(backup_left->left), dict_item_height(backup_left->right)) + 1;

    /* return new root */
    return backup_left;
}

/* Perform left rotation on node ITEM */
struct dict_item *dict_item_rotate_left(struct dict_item *item)
{
    struct dict_item *backup_right = item->right;
    struct dict_item *backup_left = backup_right->left;

    /* Perform rotation */
    backup_right->left = item;
    item->right = backup_left;

    /* Update the heights */
    item->height = max(dict_item_height(item->left), dict_item_height(item->right)) + 1;
    backup_right->height = max(dict_item_height(backup_right->left), dict_item_height(backup_right->right)) + 1;

    /* return new root */
    return backup_right;
}

/* Insert dictionary item ITEM to a dictionary root items ROOT. If the root
 * is NULL then ITEM will be the root of the dictionary */
struct dict_item *dict_item_insert(struct dict_item *root, struct dict_item *item) 
{
    /* Set root for the first time or assign to root->left or root->right */
    if(root == NULL) {
        return item;
    }

    /* We compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(root->term, item->term) > 0) {
        if(root->left == NULL) {
            item->parent = root;
        }
        root->left = dict_item_insert(root->left, item);
    } else if(strcmp(root->term, item->term) < 0) {
        if(root->right == NULL) {
            item->parent = root;
        }
        root->right = dict_item_insert(root->right, item);
    }

    /* Update the height of the ancestor node */
    root->height = max(dict_item_height(root->left), dict_item_height(root->right)) + 1;
    
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

/* Check ITEM if it exists in dictionary DICT */
int dict_item_exists(struct dict_item *dict, struct dict_item *item)
{
    if(dict == NULL) {
        return FALSE;
    }

    /* we compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(dict->term, item->term) == 0) {
        return TRUE;
    } else if(strcmp(dict->term, item->term) < 0) {
        return dict_item_exists(dict->right, item);
    } else if(strcmp(dict->term, item->term) > 0) {
        return dict_item_exists(dict->left, item);
    }

    return FALSE;
}

/* Read each word from a file stream FP. Stem the word and insert the word
 * to the dictionary DICT. */
struct dictionary *dict_build_from_file(struct dictionary *dict, FILE *fp)
{
    /* To save a discovered token temporary */
    char token[MAX_TOKEN_CHAR];
    int ti = 0; 

    /* Read every char C in the file FP until end of file */
    int c;
    while((c = fgetc(fp)) != EOF) {
        /* We only care if the char is alphabet */
        if(isalpha(c)) {
            /* build a token, convert all char to lowercase */
            if (ti < (MAX_TOKEN_CHAR - 2)) {
                token[ti] = tolower(c);
            }
            /* increment the token index */
            ti++;
        } else if(isspace(c)) {
            /* If the next char is a whitespace, then close the previous 
             * discovered token then create a new vocab and add it to a 
             * dictionary DICT */ 
            if(ti != 0 && ti < (MAX_TOKEN_CHAR-2)) {
                /* terminate the current token */
                token[ti] = '\0';
                /* Save the token to a t variable, to avoid re-used token pointer
                 * across a dictionary items. */
                char *t = (char *)malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(t, token);
                struct dict_item *vocab = dict_item_new(t);
                
                /* count root */
                if(dict->root == NULL) {
                    dict->nitems = 1;
                }
                /* Insert dictionary item VOCAB to a dictionary DICT */
                dict->root = dict_item_insert(dict->root, vocab);
                /* for root vocab */
                if(dict->nitems == 1) {
                    vocab->index = dict->nitems;
                }
                if(vocab->parent) {
                    /* increment nitems, update vocab */
                    dict->nitems += 1;
                    vocab->index = dict->nitems;
                }
                /* All allocated memory in here will be freed when dictionary
                 * DICT is destroyed */
            }
            /* Reset the token index */
            ti = 0;
        }
    }

    return dict;
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

    /* corpus_dir is required */
    if(!opts.corpus_dir) {
        fprintf(stderr, "-c options is required. Please see %s --help\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Skip building stop words dictionary if the FILE is not provided */
    if(!opts.stopwords_file) {
        printf("sayoeti: %s\n", "stop words file is not specified.");
        printf("sayoeti: %s\n", "skipping process building stop words dictionary");
    }

    /* Build stop words dictionary if the FILE is specified */
    struct dictionary *stopwords_dict = NULL;
    if(opts.stopwords_file) {
        printf("sayoeti: build stop words dictionary from %s\n", opts.stopwords_file);
        stopwords_dict = dict_init();

        /* Read the file */
        FILE *fp = fopen(opts.stopwords_file, "r");
        if(fp == NULL) {
            fprintf(stderr, "Couldn't open the file %s; %s\n", opts.stopwords_file, strerror(errno));
            /* exit as a failure to notify that we cannot open the stop words file */
            exit(EXIT_FAILURE);
        }

        /* Build dictionary from a file */
        stopwords_dict = dict_build_from_file(stopwords_dict, fp);

        /* Close the file; only display info if error */
        if(fclose(fp) != 0) {
            fprintf(stderr, "Couldn't close the file: %s; %s\n", opts.stopwords_file, strerror(errno));
        }

        /* Notify that stop words dictionary has created */
        printf("sayoeti: %s\n", "stop words dicitonary is created.");
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