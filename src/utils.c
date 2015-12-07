/* Sayoeti Utilities
 * Collection of common function used across sayoeti code base.
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
#include <ctype.h>

#include "utils.h"

/* util_tokenf: get each word separated by space on the file FP.
 * It returns 0 if the EOF is reached and it's guarantee that
 * no token with length more that MAXTOKEN are returned */
int util_tokenf(char token[], int maxtoken, FILE *fp)
{
    /* Keep track the token index */
    int ti = 0;

    /* Read all char C before space */
    int c;
    while((c = fgetc(fp)) != EOF) {
        
        /* Stop reading if we encounter a space */
        if(isspace(c) || !isalnum(c)) {
            /* But we keep reading if we don't get any token yet */
            if(ti == 0) continue;

            /* If the token length is exceeded, throw the token, 
             * and get the next one */
            if(ti > maxtoken-1) {
                ti = 0;
                continue;
            }

            /* If token is fine, then we stop reading.
             * and return the token */
            break;
        }

        /* Save the current character C to token TOKEN */
        if(isalnum(c) && (ti < maxtoken-1)) {
            token[ti] = tolower(c);
        }

        /* Increase the index of token */
        ti++;
    }
    
    /* Terminate the current token */
    if(ti > 0 && ti < maxtoken-1) {
        token[ti] = '\0';
    }

    return ti;
}

/* util_tokenb: get each word separated by space on from the BUFFER.
 * It's return the last accessed index buffer. Since we don't return
 * the length of token we guarantee that the returned token is not 
 * exceed the MAXTOKEN. We assume that the buffer is terminated by '\r'. */
int util_tokenb(char token[], int maxtoken, int indexbuf, char *buffer)
{

    /* Keep track the index of token */
    int ti = 0;

    /* Read each char until '\r' */
    int c;
    while((c = buffer[indexbuf]) != '\r') {
        /* Stop reading if we encounter a space */
        if(isspace(c) || !isalnum(c)) {
            /* But we keep reading if we don't get any token yet */
            if(ti == 0) {
                indexbuf += 1;
                continue;
            };

            /* If the token length is exceeded, throw the token, 
             * and get the next one */
            if(ti > maxtoken-1) {
                ti = 0;
                indexbuf += 1;
                continue;
            }

            /* If token is fine, then we stop reading.
             * and return the token */
            break;
        }

        /* Save the current character C to token TOKEN */
        if(isalnum(c) && (ti < maxtoken-1)) {
            token[ti] = tolower(c);
        }

        /* Increase the index of token */
        ti++;

        /* Increase the index of buffer */
        indexbuf += 1;
    }
    
    /* Terminate the current token */
    if(ti > 0 && ti < maxtoken-1) {
        token[ti] = '\0';
    }

    /* Return the last index of accessed buffer */
    return indexbuf;
}

/* util_max: return the biggest element from a and b */
int util_max(int a, int b)
{
    return (a > b) ? a : b;
}