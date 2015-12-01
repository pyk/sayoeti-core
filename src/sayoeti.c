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
 * We use binary search tree here. for fastest lookup and insert.
 ****************************/
#define TRUE 1
#define FALSE 0
/* This is the maximum number of characters in one token.
 * see: https://en.wikipedia.org/wiki/Longest_Words#Indonesian */
#define MAX_TOKEN_CHAR 31 

/* dict_item represent a node of Binary Search Tree(BST) that we used as a 
 * underlying data structure for building dictionary.
 * Why? See https://en.wikipedia.org/wiki/Binary_search_tree */
struct dict_item {
    /* basic info for each item in dictionary */
    long index;
    char *term;

    /* for init & root dictionary; initializer node is to make sure that we only 
     * select the root node only once in the insert operation */
    int is_init;
    int is_root;
    
    /* keep track of how many item in this dictionary. All item except the root
     * will be zeroed */
    long nitems;

    struct dict_item *left, *right;
};

/* Initialize the root of dictionary */
struct dict_item *dict_init(void)
{
    struct dict_item *root = (struct dict_item *)malloc(sizeof(struct dict_item));
    root->is_init = TRUE;
    root->is_root = FALSE;
    return root;
}

/* Print all items in dictionary DICT */
void dict_printout(struct dict_item *dict)
{
    if (dict->is_root) {
        printf("DICT: index: %li root: %s nitems: %li\n", dict->index, dict->term, dict->nitems);
    }
    if(dict->left) dict_printout(dict->left);
    printf("%li:%s\n", dict->index, dict->term);
    if(dict->right) dict_printout(dict->right);
}

/* Destroy the dictionary DICT */
void dict_destroy(struct dict_item *dict) 
{
    if(dict == NULL) {
        return;
    } 

    dict_destroy(dict->left);
    dict_destroy(dict->right);

    free(dict->term);
    free(dict);
}

/* Create new dictionary item with term TERM */
struct dict_item *dict_item_new(char *term)
{
    struct dict_item *item = (struct dict_item *)malloc(sizeof(struct dict_item));
    item->left = item->right = NULL;
    item->index = 0;
    item->term = term;
    item->is_init = FALSE;
    item->is_root = FALSE;
    item->nitems = 0;
    return item;
}

/* Insert dictionary item ITEM to a dictionary DICT. If the dictionary
 * is NULL then NODE will be the root of dictionary */
struct dict_item *dict_item_insert(struct dict_item *dict, struct dict_item *item, int *is_new) 
{
    /* Insert to left or right item */
    if(dict == NULL) {
        *is_new = TRUE;
        return item;
    }

    /* Check if DICT is an initializer item; then make the item ITEM as a root
     * of the dictionary */
    if(dict->is_init) {
        item->index = 1;
        item->is_root = TRUE;
        item->nitems = 1;
        return item;
    }

    /* we compare each word in lexicographical order. left leaf start from 'a' */
    if(strcmp(dict->term, item->term) > 0) {
        dict->left = dict_item_insert(dict->left, item, is_new);
    } else if(strcmp(dict->term, item->term) < 0) {
        dict->right = dict_item_insert(dict->right, item, is_new);
    }

    return dict;
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
struct dict_item *dict_build_from_file(struct dict_item *dict, FILE *fp)
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
                
                /* Insert dictionary item VOCAB to a dictionary DICT */
                int is_new_item = FALSE;
                dict = dict_item_insert(dict, vocab, &is_new_item);
                if(is_new_item) {
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
    struct dict_item *stopwords_dict = NULL;
    if(opts.stopwords_file) {
        printf("sayoeti: build stop words dictionary from %s\n", opts.stopwords_file);
        stopwords_dict = dict_init();

        /* Insert the first item to a stopwords dictionary; we choose mmmmm because m is 
         * the 13th of alphabet. so we have nice binary search tree here */
        struct dict_item *vocab = dict_item_new("mmmmm");
        int is_new_item = FALSE;
        stopwords_dict = dict_item_insert(stopwords_dict, vocab, &is_new_item);
        if(is_new_item) {
            /* Increment nitems, update vocab */
            stopwords_dict->nitems += 1;
            vocab->index = stopwords_dict->nitems;
        }

        /* Read the file */
        FILE *fp = fopen(opts.stopwords_file, "r");
        if(fp == NULL) {
            fprintf(stderr, "Couldn't open the file %s; %s\n", opts.stopwords_file, strerror(errno));
            /* exit as a failure to notify that we cannot open the stop words file */
            exit(EXIT_FAILURE);
        }

        /* Build dictionary from file */
        stopwords_dict = dict_build_from_file(stopwords_dict, fp);

        /* Close the file; only display info if error */
        if(fclose(fp) != 0) {
            fprintf(stderr, "Couldn't close the file: %s; %s\n", opts.stopwords_file, strerror(errno));
        }

        /* Notify that stop words dictionary has created */
        printf("sayoeti: %s\n", "stop words dicitonary is created.");
    }

    if(stopwords_dict) {
        struct dict_item *vocab = dict_item_new("tidaklah");
        if(dict_item_exists(stopwords_dict, vocab)) {
            printf("%s\n", "EXISTS");
        } else {
            printf("%s\n", "NOT EXISTS");
        }
    }

    printf("CORPUS = %s\n", opts.corpus_dir);
    printf("STOPWORDS = %s\n", opts.stopwords_file);
    printf("PORTS = %s\n", opts.port);

    return 0;
}