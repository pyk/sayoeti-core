# Sayoeti Core
Sayoeti is the PR's assistant of [Komisi Pemberantasan Korupsi][KPK](KPK) that 
powered by Artificial Intelligence. He helps KPK by watching all mass media in 
Indonesia and provide sentiment analysis of the mass media.

[KPK]: http://www.kpk.go.id/splash/

This is one of component that Sayoeti built on. Basically, `sayoeti-core` is 
created to answer the following question:

    Human: Given a document, is this document about corruption news in Indonesia? (Yes/No)
    Sayoeti: Yes, it is.

Learn more here [https://sayoeti.xyz](https://sayoeti.xyz) (Indonesian).

## Requirements
We use supervised learning method here. Sayoeti need to learn from corpus
of corruption news first. Example of corpus from Kompas.com and Liputan6.com
can be found [here][corpus].

To decrease the bias we need to remove commonly used words in Indonesian
like `dan` & `di` that have no meaning in our context. To do this, use
[Indonesian stopwords][sw].

Sayoeti only tested on `x86_64`. Compiled using `gcc`.

[corpus]: https://www.dropbox.com/s/vuziwj3wcwfrter/example-corpus.tar.gz?dl=0
[sw]: https://sites.google.com/site/kevinbouge/stopwords-lists

## Setup

Clone the repository

    git clone https://github.com/pyk/sayoeti-ai
    cd sayoeti-ai

Install dependencies

    make libmill

Build Sayoeti

    make

Run Sayoeti

    LD_LIBRARY_PATH=/usr/local/lib ./sayoeti -c /path/to/corpusdir -s /path/to/stopwords/file

Sayoeti will listening on port `9090` by default.

## Example
Running Sayoeti

    $ LD_LIBRARY_PATH=/usr/local/lib ./sayoeti -c corpus -s stopwords_id.txt 
    sayoeti: Create stop words dictionary from stopwords_id.txt
    sayoeti: stop words dictionary from stopwords_id.txt is created.
    sayoeti: Create index vocabulary from corpus corpus
    sayoeti: Index vocabulary from corpus corpus created.
    sayoeti: compute global IDF for each term in index vocabulary
    sayoeti: create a problem
    *
    optimization finished, #iter = 9
    obj = 278.605220, rho = 23.603209
    nSV = 24, nBSV = 23
    sayoeti: listening on port :9090

Throw document for Sayoeti to read

    $ telnet localhost 9090
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    202 OK sayoeti ready

## License

Copyright 2015 Bayu Aldi Yansyah <bayualdiyansyah@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.