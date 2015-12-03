/* Sayoeti CLI (command line interface)
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

/* TODO(pyk): copy all char* that passed as argument of function to 
 * new memory to prevent us freeing memory from the outside function */

#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "utils.h"
#include "dict.h"
#include "stopwords.h"

#include "../deps/libsvm/svm.h"

/****************************
 * Corpus
 ****************************/

/* corpus_doc_item: represents unique token in the document */
struct corpus_doc_item {
    /* The index of item in corpus dictionary */
    long index;

    /* This will enable us to lookup global information like IDF
     * from corpus dicitonary */
    char *term;

    /* Keep track of inserted document */
    int is_inserted;
    
    /* Keep track of the TF (term frequency) int the document */
    int frequency;

    /* Height, Left and Right for the AVL tree */
    int height;
    struct corpus_doc_item *left, *right;
};

/* corpus_doc_item_init: initializes new corpus document item */
struct corpus_doc_item *corpus_doc_item_init(long index, char *term)
{
    /* Allocate memory for current document item */
    struct corpus_doc_item *cdoci = (struct corpus_doc_item *)malloc(sizeof(struct corpus_doc_item));
    if(cdoci == NULL) {
        return NULL;
    }
    
    /* Save the term to a t variable, to avoid deletion of index vocabulary term */
    char *t = (char *)malloc(sizeof(char) * (strlen(term) + 1));
    if(t == NULL) {
        return NULL;
    }

    /* Copy TERM to T */
    strcpy(t, term);

    cdoci->index = index;
    cdoci->term = t;
    cdoci->frequency = 1;
    cdoci->is_inserted = FALSE;
    cdoci->height = 1;

    return cdoci;
}

/* corpus_doc_item_destroy: free all allocated memory for document item 
 * CDOCI */
void corpus_doc_item_destroy(struct corpus_doc_item *cdoci)
{
    if(cdoci == NULL) {
        return;
    }

    free(cdoci->term);
    free(cdoci);
}


/* corpus_doc_item_height_max: returns the maximum value of H1 and H2 */
int corpus_doc_item_height_max(int h1, int h2) {
    return (h1 > h2) ? h1 : h2;
}

/* corpus_doc_item_height: get the height of corpus document item ITEM */
int corpus_doc_item_height(struct corpus_doc_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return item->height;
}

/* corpus_doc_item_get_balance: get the balance factor of corpus document 
 * item ITEM */
int corpus_doc_item_get_balance(struct corpus_doc_item *item)
{
    if(item == NULL) {
        return 0;
    }
    return corpus_doc_item_height(item->left) - corpus_doc_item_height(item->right);
}

/* corpus_doc_item_rotate_right: Performs tree operation right rotation on 
 * corpus document item ITEM. It returns rotated document item. */
struct corpus_doc_item *corpus_doc_item_rotate_right(struct corpus_doc_item *item)
{
    struct corpus_doc_item *backup_left = item->left;
    struct corpus_doc_item *backup_right = backup_left->right;

    /* Perform rotation */
    backup_left->right = item;
    item->left = backup_right;

    /* Update the heights */
    int hil = corpus_doc_item_height(item->left);
    int hir = corpus_doc_item_height(item->right);
    item->height = corpus_doc_item_height_max(hil, hir) + 1;

    int bll = corpus_doc_item_height(backup_left->left);
    int blr = corpus_doc_item_height(backup_left->right);
    backup_left->height = corpus_doc_item_height_max(bll, blr) + 1;

    /* Return rotated item */
    return backup_left;
}

/* corpus_doc_item_rotate_left: Performs tree operation left rotation on 
 * dictionary item ITEM. It returns rotated dicitonary item. */
struct corpus_doc_item *corpus_doc_item_rotate_left(struct corpus_doc_item *item)
{
    struct corpus_doc_item *backup_right = item->right;
    struct corpus_doc_item *backup_left = backup_right->left;

    /* Perform rotation */
    backup_right->left = item;
    item->right = backup_left;

    /* Update the heights */
    int hil = corpus_doc_item_height(item->left);
    int hir = corpus_doc_item_height(item->right);
    item->height = corpus_doc_item_height_max(hil, hir) + 1;

    int brl = corpus_doc_item_height(backup_right->left);
    int brr = corpus_doc_item_height(backup_right->right);
    backup_right->height = corpus_doc_item_height_max(brl, brr) + 1;

    /* Return rotated item */
    return backup_right;
}

/* corpus_doc_item_insert: insert new document item to the document root ROOT;
 * increase the frequency if the ITEM is already exists in ROOT */
struct corpus_doc_item *corpus_doc_item_insert(struct corpus_doc_item *root, struct corpus_doc_item *item) 
{
    /* Set root for the first time or assign to root->left or root->right */
    if(root == NULL) {
        item->is_inserted = TRUE;
        return item;
    }

    /* Build Binary Search Tree based on item->index */
    if(item->index == root->index) {
        /* increase the frequency of the root */
        root->frequency += 1;
    } else if(item->index < root->index) {
        root->left = corpus_doc_item_insert(root->left, item);
    } else if(item->index > root->index) {
        root->right = corpus_doc_item_insert(root->right, item);
    }

    /* Update the height of the ancestor node */
    int hrl = corpus_doc_item_height(root->left);
    int hrr = corpus_doc_item_height(root->right);
    root->height = corpus_doc_item_height_max(hrl, hrr) + 1;
    
    /* Get the balance factor to check wether tree is balance or unbalanced */
    int balance = corpus_doc_item_get_balance(root);

    /* 4 case if the tree is unbalanced 
     * Case 1: Left left case */
    if(balance > 1 && (item->index < root->index)) {
        /* Perform right rotation */
        return corpus_doc_item_rotate_right(root);
    }

    /* Case 2: Right right case */
    if(balance < -1 && (item->index > root->index)) {
        /* Perform left rotation */
        return corpus_doc_item_rotate_left(root);
    }

    /* Case 3: Right left case */
    if(balance < -1 && (item->index < root->index)) {
        root->right = corpus_doc_item_rotate_right(root->right);
        return corpus_doc_item_rotate_left(root);
    }

    /* Case 4: Left right case */
    if(balance > 1 && (item->index > root->index)) {
        root->left = corpus_doc_item_rotate_left(root->left);
        return corpus_doc_item_rotate_right(root);
    }

    return root;
}

/* corpus_doc_item_print: print the sparse vector representation of the corpus
 * document */
void corpus_doc_item_print(struct corpus_doc_item *root)
{
    if(root->left) corpus_doc_item_print(root->left);
    printf("%li:%d:%s ", root->index, root->frequency, root->term);
    if(root->right) corpus_doc_item_print(root->right);
}

/* corpus_doc: represents the corpus document */
struct corpus_doc {
    /* The psth to the document */
    char *path;

    /* Keep track of how many items in the document */
    long nitems; 

    /* Local dictionay of docment; ordered by the number of
     * index in corpus dictionary */
    struct corpus_doc_item *root;
};

/* corpus_doc_init: initialize new corpus document */
struct corpus_doc *corpus_doc_init(char *path)
{
    struct corpus_doc *cdoc = (struct corpus_doc *)malloc(sizeof(struct corpus_doc));
    if(cdoc == NULL) {
        return NULL;
    }

    /* Copy *path to new memory, so this *cdoc is self-containable. Not depending
     * to another memory address. This is fucking exciting */
    char *p = (char *)malloc(sizeof(char) * (strlen(path) + 1));
    if(p == NULL) {
        return NULL;
    }

    /* Copy TERM to T */
    strcpy(p, path);

    cdoc->path = p;
    cdoc->nitems = 0;
    cdoc->root = NULL;

    return cdoc;
}

/* corpus_doc_create: create document vector representation using TF(term 
 * frequency). */
struct corpus_doc *corpus_doc_create(char *path, FILE *fp, struct dict *corpus)
{
    /* Create corpus doc */
    struct corpus_doc *cdoc = corpus_doc_init(path);
    if(cdoc == NULL) {
        return NULL;
    }

    /* Read every token in the file FP and populate the doc items */
    char token[MAX_TOKEN_CHAR];
    int lentoken;
    while((lentoken = util_tokenf(token, MAX_TOKEN_CHAR, fp)) != 0) {
        /* The token length is exceeded, so skip the token we get the next 
         * token instead */
        if(lentoken > MAX_TOKEN_CHAR-1) {
            continue;
        }

        /* Search the TOKEN in CORPUS dictionary, if the DITEM is NULL then
         * continue to the next token */
        struct dict_item *ditem = dict_item_search(corpus->root, token);
        if(ditem == NULL) {
            continue;
        }

        /* Create new document item */
        struct corpus_doc_item *cdoci = corpus_doc_item_init(ditem->index, ditem->term);
        if(cdoci == NULL) {
            /* We can't skip this, because the doc item is so important. 
             * so let's tell the caller */
            return NULL;
        }

        /* Insert document item to the root document */
        cdoc->root = corpus_doc_item_insert(cdoc->root, cdoci);
        /* If item is inserted to root, then increase the number of document */
        if(cdoci->is_inserted) {
            cdoc->nitems += 1;
        }

        /* if item is not inserted; it's mean that there are exists item with 
         * the same index as this. just increment the previous item frequency 
         * and remove this item */
        if(!cdoci->is_inserted) {
            corpus_doc_item_destroy(cdoci);
        }
    }           

    /* Return populated document */
    return cdoc;
}

/* corpus_docs_init: creates representation of each document in the directory
 * path DIRPATH as a *corpus_doc */
struct corpus_doc **corpus_docs_init(char *dirpath, struct dict *corpus)
{
    /* Allocate the memory for the array of *CORPUS_DOC */
    struct corpus_doc **cdocs = (struct corpus_doc **)malloc(corpus->ndocs * sizeof(struct corpus_doc *));
    if(cdocs == NULL) {
        return NULL;
    }

    /* Open the dictionary */
    DIR *dir;
    dir = opendir(dirpath);
    if(dir == NULL) {
        return NULL;
    }

    /* Keep track of the number of documents; make sure we don't
     * overflow the CDOCS array */
    int ndocs = 0;

    /* Scan all files inside directory DIR */
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        /* We only care if the ENT is a regular file */
        if(ent->d_type != DT_REG) {
            /* Skip this file */
            continue;
        }

        /* Get the path to the file ENT */
        char *path_to_file = (char *)malloc(sizeof(char) * (strlen(dirpath) + strlen(ent->d_name) + 2));
        if(path_to_file == NULL) {
            return NULL;
        }

        /* Specify relative path to the file */
        if(dirpath[strlen(dirpath)-1] == '/') {
            sprintf(path_to_file, "%s%s", dirpath, ent->d_name);
        } else {
            sprintf(path_to_file, "%s/%s", dirpath, ent->d_name);
        }

        /* Read the file */
        FILE *fp = fopen(path_to_file, "r");
        if(fp == NULL) {
            fprintf(stderr, "Couldn't open the file %s; %s\n", path_to_file, strerror(errno));
            /* skip the file; return the beginning of the while loop
             * to open the next file */
            continue;
        }

        /* We can't continue if the array is full */
        if(ndocs > corpus->ndocs) {
            break;
        }

        /* Creates corpus_doc representation for each document */
        struct corpus_doc *cdoc = corpus_doc_create(path_to_file, fp, corpus);
        if(cdoc == NULL) {
            return NULL;
        }

        /* Save the document */
        cdocs[ndocs] = cdoc;

        /* Close the file */
        if(fclose(fp) != 0) {
            return NULL;
        }

        /* We don't need the variable again */
        free(path_to_file);

        /* Increase the number document we scan */
        ndocs += 1;    
    }

    /* Close the opened directory DIR */
    if(closedir(dir) != 0) {
        return NULL;
    }

    return cdocs;
}

/* corpus_dict_create: creates dictionary CORPUS from corpus DIR.
 * If the words is not in the dictionary EXC then the words is inserted to
 * dictionary CORPUS.
 * 
 * Returns NULL if only if error happen and ERRNO will be set to last
 * error. */
struct dict *corpus_dict_create(char *dirpath, struct dict *exc, struct dict *corpus)
{
    /* Initialize the dictionary */
    corpus = dict_new(dirpath);
    if(corpus == NULL) {
        return NULL;
    }

    /* Read all files in DIRPATH and build dictionary from words inside the file. 
     * It assume that all files inside directory DIRPATH is text based. */
    DIR *dir;
    dir = opendir(dirpath);
    if(dir == NULL) {
        return NULL;
    }

    /* Scan all files inside directory DIR */
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        /* We only care if the ENT is a regular file */
        if(ent->d_type != DT_REG) {
            /* Skip this file */
            continue;
        }

        /* Get the path to the file ENT */
        char *path_to_file = (char *)malloc(sizeof(char) * (strlen(dirpath) + strlen(ent->d_name) + 2));
        if(path_to_file == NULL) {
            return NULL;
        }

        /* Specify relative path to the file */
        if(dirpath[strlen(dirpath)-1] == '/') {
            sprintf(path_to_file, "%s%s", dirpath, ent->d_name);
        } else {
            sprintf(path_to_file, "%s/%s", dirpath, ent->d_name);
        }

        /* Read the file */
        FILE *fp = fopen(path_to_file, "r");
        if(fp == NULL) {
            fprintf(stderr, "Couldn't open the file %s; %s\n", path_to_file, strerror(errno));
            /* skip the file; return the beginning of the while loop
             * to open the next file */
            continue;
        }
        
        /* Increase the number document we scan */
        corpus->ndocs += 1;

        /* Read every word in file FP, stem and insert each word to a dictionary */
        corpus = dict_populatef(fp, exc, corpus);
        if(corpus == NULL) {
            return NULL;
        }

        /* Close the file; only display info */
        if(fclose(fp) != 0) {
            return NULL;
        }

        /* We don't need the variable again */
        free(path_to_file);    
    }

    /* Close the opened directory DIR */
    if(closedir(dir) != 0) {
        return NULL;
    }

    return corpus;
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
    struct dict *stopw_dict = NULL;
    if(opts.stopwords_file) {
        printf("sayoeti: Create stop words dictionary from %s\n", opts.stopwords_file);
        stopw_dict = stopw_dict_create(opts.stopwords_file);
        if(stopw_dict == NULL) {
            fprintf(stderr, "sayoeti: Couldn't create dicitonary from file: %s; %s\n", 
                opts.stopwords_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
        dict_printout(stopw_dict);
        printf("sayoeti: stop words dictionary from %s is created.\n", opts.stopwords_file);
    }

    /* Create index vocabulary from corpus */
    struct dict *index = NULL;
    printf("sayoeti: Create index vocabulary from corpus %s\n", opts.corpus_dir);
    index = corpus_dict_create(opts.corpus_dir, stopw_dict, index);
    if(index == NULL) {
        fprintf(stderr, "sayoeti: Couldn't create index vocabulary from corpus: %s; %s\n", 
            opts.corpus_dir, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("sayoeti: Index vocabulary from corpus %s created.\n", opts.corpus_dir);
    // dict_printout(index);

    /* Create sparse representation of corpus documents */
    struct corpus_doc **cdocs = corpus_docs_init(opts.corpus_dir, index);
    int cdi = 0;
    for(cdi = 0; cdi < index->ndocs; cdi++) {
        printf("sayoeti: corpus document %d; %li words %s\n", cdi+1, cdocs[cdi]->nitems, cdocs[cdi]->path);
        corpus_doc_item_print(cdocs[cdi]->root);
        printf("\n");
    }

    /* Create a SVM parameter */
    struct svm_parameter param;
    param.svm_type = ONE_CLASS;
    param.kernel_type = RBF;
    param.degree = 3;
    param.gamma = 0;
    param.coef0 = 0;
    param.nu = 0.5;
    param.cache_size = 100;
    param.C = 1;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 1;
    param.probability = 0;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;

    printf("DEBUG: param type: %d\n", param.svm_type);
    /* Create the training model */
    // struct svm_problem *svmp = train_problem_create(opts.corpus_dir, index);

    printf("PORTS = %s\n", opts.port);
    dict_destroy(stopw_dict);
    dict_destroy(index);
    return 0;
}