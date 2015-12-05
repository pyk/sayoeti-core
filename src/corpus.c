/* Sayoeti Corpus
 * Collection of common function to manage the corpus of sayoeti.
 * - Create index vocabulary from corpus
 * - Create vector representation sparse vector of document using
 *   TF(term frequency) 
 * - Compute IDF(Inverse document frequency) for each term in 
 *   Index vocabulary
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
#include <dirent.h>
#include <errno.h>

#include "dict.h"
#include "corpus.h"
#include "utils.h"

/* corpus_doc_item_new: initializes new corpus document item */
struct corpus_doc_item *corpus_doc_item_new(long index, char *term)
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

    /* Set initials value */
    cdoci->index = index;
    cdoci->term = t;
    cdoci->frequency = 1;
    cdoci->is_inserted = FALSE;
    cdoci->height = 1;
    cdoci->left = NULL;
    cdoci->right = NULL;

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

/* corpus_doc_item_rotate_right: Performs AVL tree right rotation on 
 * document item ITEM. It returns right rotated document item. */
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
    item->height = util_max(hil, hir) + 1;

    int bll = corpus_doc_item_height(backup_left->left);
    int blr = corpus_doc_item_height(backup_left->right);
    backup_left->height = util_max(bll, blr) + 1;

    /* Return rotated item */
    return backup_left;
}

/* corpus_doc_item_rotate_left: Performs AVL tree right rotation on 
 * document item ITEM. It returns right rotated document item. */
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
    item->height = util_max(hil, hir) + 1;

    int brl = corpus_doc_item_height(backup_right->left);
    int brr = corpus_doc_item_height(backup_right->right);
    backup_right->height = util_max(brl, brr) + 1;

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
    }else if(item->index < root->index) {
        root->left = corpus_doc_item_insert(root->left, item);
    }else if(item->index > root->index) {
        root->right = corpus_doc_item_insert(root->right, item);
    }

    /* Update the height of the ancestor node */
    int hrl = corpus_doc_item_height(root->left);
    int hrr = corpus_doc_item_height(root->right);
    root->height = util_max(hrl, hrr) + 1;
    
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
    if(root == NULL) return;
    if(root->left) corpus_doc_item_print(root->left);
    printf("%li:%d:%s ", root->index, root->frequency, root->term);
    if(root->right) corpus_doc_item_print(root->right);
}

/* corpus_doc_item_exists: check wether the INDEX of term is exists or not
 * in the document root ROOT. It returns positive integer is the INDEX is 
 * found, otherwise 0 will returned */
int corpus_doc_item_exists(struct corpus_doc_item *root, long index)
{
    /* ITEM not exists in ROOT */
    if(root == NULL) return 0;
    if(index == root->index) return 1;
    if(index < root->index) return corpus_doc_item_exists(root->left, index);
    if(index > root->index) return corpus_doc_item_exists(root->right, index);

    return 0;
}

/* corpus_doc_new: initialize new corpus document */
struct corpus_doc *corpus_doc_new(char *path)
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

/* corpus_doc_createf: create document vector representation using TF(term 
 * frequency) from file FP. */
struct corpus_doc *corpus_doc_createf(char *path, FILE *fp, struct dict *corpus)
{
    /* Create corpus doc */
    struct corpus_doc *cdoc = corpus_doc_new(path);
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
        struct corpus_doc_item *cdoci = corpus_doc_item_new(ditem->index, ditem->term);
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


/* corpus_doc_createb: create document vector representation using TF(term 
 * frequency) from buffer BUF. */
struct corpus_doc *corpus_doc_createb(int lenbuf, char *buf, struct dict *index)
{
    /* Create corpus doc */
    struct corpus_doc *cdoc = corpus_doc_new("buffer");
    if(cdoc == NULL) {
        return NULL;
    }

    /* Read every token in the file FP and populate the doc items */
    char token[MAX_TOKEN_CHAR];
    int indexbuf = 0;
    while((indexbuf = util_tokenb(token, MAX_TOKEN_CHAR, indexbuf, buf)) < (lenbuf-1)) {
        /* Search the TOKEN in CORPUS dictionary, if the DITEM is NULL then
         * continue to the next token */
        struct dict_item *ditem = dict_item_search(index->root, token);
        if(ditem == NULL) {
            continue;
        }

        /* Create new document item */
        struct corpus_doc_item *cdoci = corpus_doc_item_new(ditem->index, ditem->term);
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

        /* Ff item is not inserted; it's mean that there are exists item with 
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
        struct corpus_doc *cdoc = corpus_doc_createf(path_to_file, fp, corpus);
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

/* corpus_index: index all words in DIRPATH and return new CORPUS 
 * dictionary. Any words in EXC dictionary will not indexed.
 * Returns NULL if only if error happen and ERRNO will be set to last
 * error. */
struct dict *corpus_index(char *dirpath, struct dict *exc)
{
    /* Initialize the dictionary */
    struct dict *corpus = dict_new(dirpath);
    if(corpus == NULL) {
        return NULL;
    }

    /* Open the directory */
    DIR *dir = opendir(dirpath);
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

        /* Populate CORPUS dictionary */
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

/* compute_index_idf: iterate over all items in index vocabulary INDEX; 
 * then perform check for each document in CDOCS; if the ROOT index 
 * is exists then increase the ROOT ndocs; */
void corpus_index_idf(int ndocs, struct corpus_doc **cdocs, struct dict_item *root)
{
    if(root == NULL) return;
    if(root->left) corpus_index_idf(ndocs, cdocs, root->left);
    
    int cdi;
    for(cdi = 0; cdi < ndocs; cdi++) {
        /* check if the root->index is exists in document cdoc */
        int exists = corpus_doc_item_exists(cdocs[cdi]->root, root->index);
        if(exists) {
            root->ndocs += 1;
        }
    }

    if(root->right) corpus_index_idf(ndocs, cdocs, root->right);
}