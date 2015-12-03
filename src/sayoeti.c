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
#include <errno.h>

#include "utils.h"
#include "dict.h"
#include "stopwords.h"
#include "corpus.h"

#include "../deps/libsvm/svm.h"

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
    printf("sayoeti: Create index vocabulary from corpus %s\n", opts.corpus_dir);
    struct dict *index = corpus_index(opts.corpus_dir, stopw_dict);
    if(index == NULL) {
        fprintf(stderr, "sayoeti: Couldn't create index vocabulary from corpus: %s; %s\n", 
            opts.corpus_dir, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("sayoeti: Index vocabulary from corpus %s created.\n", opts.corpus_dir);
    dict_printout(index);

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