/* Sayoeti Trainer
 * Collection of common function to teach Sayoeti.
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "dict.h"
#include "corpus.h"
#include "train.h"

/* train_node_create: create SVM node for each term in document root */
void train_node_create(int *svmni, int ndocs, struct corpus_doc *cdoc,
    struct corpus_doc_item *root, struct dict *index, struct svm_node *svmns)
{
    if(root == NULL) return;
    if(root->left) train_node_create(svmni, ndocs, cdoc, root->left, index, svmns);

    /* Get the term from index vocab */
    struct dict_item *item = dict_item_search(index->root, root->term);

    /* compute the TF-IDF weight */
    double tf = (double)root->frequency/cdoc->nitems;
    double idf = log((double)ndocs/(item->ndocs));
    // printf("DEBUG %s \"%s\" f %d nitems %li tf: %f - ndocs %d item->ndocs %d idf %f tf-idf %f\n",
    //     cdoc->path, root->term, root->frequency, cdoc->nitems, tf, ndocs, item->ndocs, idf, tf * idf);

    /* Save to the array of svm node */
    struct svm_node svmn = {root->index, tf*idf};
    svmns[*svmni] = svmn;

    /* Increase the svm node index */
    *svmni += 1;

    if(root->right) train_node_create(svmni, ndocs, cdoc, root->right, index, svmns);

    return;
}

/* train_problem_create: create SVM problem based on collection of copus 
 * documents CDOCS and index vocabulary INDEX */
struct svm_problem *train_problem_create(int ndocs, struct corpus_doc **cdocs, struct dict *index)
{
    /* Allocate memory for new problem */
    struct svm_problem *svmp = (struct svm_problem *)malloc(sizeof(struct svm_problem));
    if(svmp == NULL) return NULL;

    /* Set the number of training data */
    svmp->l = ndocs;
    
    /* Allocate memory for the target array */
    svmp->y = (double *)malloc(ndocs * sizeof(double));
    if(svmp->y == NULL) return NULL;
    
    /* Allocate memory for the array of svm_node array */
    svmp->x = (struct svm_node **)malloc(ndocs * sizeof(struct svm_node *));
    if(svmp->x == NULL) return NULL;

    int cdi;
    for(cdi = 0; cdi < ndocs; cdi++) {
        /* Because this is a ONE_CLASS SVM we just set the label of training 
         * data to 1 */
        svmp->y[cdi] = 1;

        /* Allocate memory for the svm_node array */
        struct svm_node *svmns = (struct svm_node *)malloc((cdocs[cdi]->nitems+1) * sizeof(struct svm_node));
        if(svmns == NULL) return NULL;
        /* Create svm node for each term in document */
        int svmni = 0; /* keep track the index of svm node */
        train_node_create(&svmni, ndocs, cdocs[cdi], cdocs[cdi]->root, index, svmns);

        /* Terminate the SVM node; Allocate memory for SVM node */
        struct svm_node svmn = {-1, 0};
        svmns[svmni] = svmn;

        svmp->x[cdi] = svmns;
    }

    return svmp;
}
