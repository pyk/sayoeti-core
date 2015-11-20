# Sayoeti
*Sayoeti* is an Artificial Intelligence that can understand & classify 
specified document is about corruption news in Indonesia or not. 

Basicly, *Sayoeti* is created to answer the following question:

    Human: Given a document, is this document about corruption news in Indonesia? (Yes/No)
    Sayoeti: Yes, it is.

## Guide

1. Train *Sayoeti*

    sayoeti train <positive example> <negative example>

2. Told *Sayoeti* to listening on PORT

    sayoeti listen PORT

3. Ask *Sayoeti*

    telnet localhost 8080
    ASK [long document here] # press enter
    ANS Y