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

#ifndef CORPUS_H
#define CORPUS_H
 
/* corpus_doc_item: represents unique token in the document */
struct corpus_doc_item {
    /* The index of item in corpus dictionary */
    long index;

    /* This will enable us to lookup global information like IDF
     * from corpus dictionary */
    char *term;

    /* Keep track of inserted document */
    int is_inserted;
    
    /* Keep track of the TF (term frequency) int the document */
    int frequency;

    /* Height, Left and Right for the AVL tree */
    int height;
    struct corpus_doc_item *left, *right;
};

/* corpus_doc: represents the corpus document */
struct corpus_doc {
    /* The psth to the document */
    char *path;

    /* Keep track of how many items in the document */
    long nitems; 

    /* Local dictionry of docment; ordered by the number of
     * index in index vocabulary */
    struct corpus_doc_item *root;
};

/* Prototypes */
struct corpus_doc_item *corpus_doc_item_new(long index, char *term);
void corpus_doc_item_destroy(struct corpus_doc_item *cdoci);
int corpus_doc_item_height(struct corpus_doc_item *item);
int corpus_doc_item_get_balance(struct corpus_doc_item *item);
struct corpus_doc_item *corpus_doc_item_rotate_right(struct corpus_doc_item *item);
struct corpus_doc_item *corpus_doc_item_rotate_left(struct corpus_doc_item *item);
struct corpus_doc_item *corpus_doc_item_insert(struct corpus_doc_item *root, struct corpus_doc_item *item);
void corpus_doc_item_print(struct corpus_doc_item *root);
int corpus_doc_item_exists(struct corpus_doc_item *root, long index);

struct corpus_doc *corpus_doc_new(char *path);
struct corpus_doc *corpus_doc_createf(char *path, FILE *fp, struct dict *index);
struct corpus_doc *corpus_doc_createb(int lenbuf, char *buf, struct dict *index);
struct corpus_doc **corpus_docs_init(char *dirpath, struct dict *index);
struct dict *corpus_index(char *dirpath, struct dict *exc);
void corpus_index_idf(int ndocs, struct corpus_doc **cdocs, struct dict_item *root);

#endif