CC = gcc
CFLAGS = -Wall -O3
DEPS = src/utils.h src/dict.h src/stopwords.h src/corpus.h src/train.h
OBJ = utils.o dict.o stopwords.o corpus.o train.o svm.o sayoeti.o

all: libsvm sayoeti

libsvm: deps/libsvm/svm.h deps/libsvm/svm.cpp
	g++ -Wall -Wconversion -O3 -fPIC -c deps/libsvm/svm.cpp

libmill:
	cd ./deps/libmill-1.2/ && \
	./configure && make && make check && \
	sudo make install && \
	cd -
	
%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

sayoeti: $(OBJ)
	g++ $(CFLAGS) -o $@ $^ -lm -lmill

clean:
	rm -f sayoeti.o utils.o dict.o stopwords.o corpus.o train.o svm.o sayoeti