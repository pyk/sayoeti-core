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

#ifndef TRAIN_H
#define TRAIN_H
#include "../deps/libsvm/svm.h"

/* Prototypes */
struct svm_problem *train_problem_create(int ndocs, struct corpus_doc **cdocs, struct dict *index);
void train_node_create(int *svmni, 
                       int ndocs, 
                       struct corpus_doc *cdoc,
                       struct corpus_doc_item *root, 
                       struct dict *index, 
                       struct svm_node *svmns);

#endif