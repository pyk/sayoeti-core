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

#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libmill.h>

#include "utils.h"
#include "dict.h"
#include "stopwords.h"
#include "corpus.h"
#include "train.h"

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
    {"listen", 'l', "PORT", 0, "Port to listen too (default: 9090)" },
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
        // dict_printout(stopw_dict);
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
    // dict_printout(index);

    /* Create sparse representation of corpus documents */
    struct corpus_doc **cdocs = corpus_docs_init(opts.corpus_dir, index);

    /* Compute global IDF for each term in index vocabulary */
    printf("sayoeti: compute global IDF for each term in index vocabulary\n");
    corpus_index_idf(index->ndocs, cdocs, index->root);
    // dict_printout(index);
    
    /* Create a SVM parameter */
    struct svm_parameter param;
    param.svm_type = ONE_CLASS;
    param.kernel_type = RBF;
    param.degree = 3;
    param.gamma = (double)1/index->nitems;
    param.coef0 = 0;
    param.nu = 0.387;
    param.cache_size = 100;
    param.C = 1;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 1;
    param.probability = 0;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;

    /* Create SVM problem based on CDOCS and index */
    printf("sayoeti: create a problem\n");
    struct svm_problem *svmp = train_problem_create(index->ndocs, cdocs, index);
    if(svmp == NULL) {
        fprintf(stderr, "sayoeti: Couldn't create SVM Problem: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* TODO(pyk): cleanup compiler warning for this function */
    /* Check the parameter */
    const char *errmsg = svm_check_parameter(svmp, &param);
    if(errmsg) {
        fprintf(stderr, "sayoeti: Parameter are not feasible: %s\n", errmsg);
        exit(EXIT_FAILURE);
    }

    /* Create the training model */
    struct svm_model *model = svm_train(svmp, &param);

    /* listening to port */
    int port = 9090;
    if(opts.port != NULL) port = atoi(opts.port);
    
    /* Set the address to bind to */
    ipaddr addr = iplocal("127.0.0.1", port, IPADDR_IPV4);
    
    /* Start listening for TCP connection */
    tcpsock listener = tcplisten(addr, 10);
    if(listener == NULL) {
        perror("sayoeti: couldn't listeing to socket");
        exit(EXIT_FAILURE);
    }
    printf("sayoeti: listening on port :%d\n", port);

    /* List of error message */
    char *greet = "202 OK sayoeti ready\r";
    char *bufferr = "500 BAD bad buffer; terminating connection.\r";
    char *cdocerr = "500 BAD cannot create corpus document; terminating connection.\r";
    char *svmnerr = "500 BAD cannot create svm node; terminating connection.\r";
    
    /* Forever listening */
    while(1) {
        tcpsock conn = tcpaccept(listener, -1);
        printf("sayoeti: new connection arrived\n");
        
        /* Send greetings */
        tcpsend(conn, greet, strlen(greet), -1);
        tcpflush(conn, -1);

        /* Get the input by client */
        char inbuf[5000];
        size_t leninbuf = tcprecvuntil(conn, inbuf, sizeof(inbuf), "\r", 1, -1);

        /* Make sure that input buffer terminated by \r */
        if(inbuf[leninbuf-1] != '\r') {
            /* Send errors & close the connection */
            tcpsend(conn, bufferr, strlen(bufferr), -1);
            tcpflush(conn, -1);
            tcpclose(conn);
        }

        /* Create new corpus document from buffer */
        struct corpus_doc *cdoc = corpus_doc_createb(leninbuf, inbuf, index);
        if(cdoc == NULL) {
            /* Send errors & close the connection */
            tcpsend(conn, cdocerr, strlen(cdocerr), -1);
            tcpflush(conn, -1);
            tcpclose(conn);
        }

        /* create new SVM node */
        /* Allocate memory for the svm_node array */
        struct svm_node *svmns = (struct svm_node *)malloc((cdoc->nitems+1) * sizeof(struct svm_node));
        if(svmns == NULL) {
            /* Send errors & close the connection */
            tcpsend(conn, svmnerr, strlen(svmnerr), -1);
            tcpflush(conn, -1);
            tcpclose(conn);
        }

        /* Create svm node for each term in document */
        int svmni = 0; /* keep track the index of svm node */
        train_node_create(&svmni, index->ndocs, cdoc, cdoc->root, index, svmns);

        /* Terminate the SVM node */
        struct svm_node svmn = {-1, 0};
        svmns[svmni] = svmn;

        /* Print vector representtion */
        int svmnpi;
        for(svmnpi = 0; svmnpi < svmni; svmnpi++) {
            printf("%d:%f ", svmns[svmnpi].index, svmns[svmnpi].value);
        }
        printf("\n");

        /* Predict the node */
        double prediction = svm_predict(model, svmns);
        char res[20];
        sprintf(res, "RES %.0f\r", prediction);

        /* Send the result */
        tcpsend(conn, res, strlen(res), -1);
        tcpflush(conn, -1);

        /* TODO(pyk): free all allocated memory in this while loop here */
        /* Terminate the connection */
        tcpclose(conn);
    }

    /* TODO(pyk) destroy the corpus doc */
    dict_destroy(stopw_dict);
    dict_destroy(index);
    return 0;
}