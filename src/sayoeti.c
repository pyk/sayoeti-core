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
    
    printf("CORPUS = %s\n", opts.corpus_dir);
    printf("STOPWORDS = %s\n", opts.stopwords_file);
    printf("PORTS = %s\n", opts.port);

    return 0;
}