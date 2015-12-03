CC = gcc
CFLAGS = -Wall -O3
DEPS = src/utils.h src/dict.h src/stopwords.h
OBJ = sayoeti.o utils.o dict.o stopwords.o svm.o

all: libsvm sayoeti

libsvm: deps/libsvm/svm.h deps/libsvm/svm.cpp
	g++ -Wall -Wconversion -O3 -fPIC -c deps/libsvm/svm.cpp

%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

sayoeti: $(OBJ)
	g++ $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f sayoeti.o utils.o dict.o stopwords.o svm.o sayoeti