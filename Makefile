CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++17
DBGFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++17 -fsanitize=undefined -fsanitize=leak -fsanitize=address -DDEBUG

all: decode encode

debug: codec.o huffman.o buffer.hpp image.o decode.cpp
	$(CXX) image.o huffman.o codec.o decode.cpp -o decode $(DBGFLAGS)

decode: codec.o huffman.o buffer.hpp image.o decode.cpp
	$(CXX) image.o huffman.o codec.o decode.cpp -o decode $(CFLAGS)

encode: codec.o huffman.o buffer.hpp image.o encode.cpp
	$(CXX) image.o huffman.o codec.o encode.cpp -o encode $(CFLAGS)

huffman.o: huffman.cpp
	$(CXX) -c -o huffman.o huffman.cpp $(CFLAGS)

codec.o: codec.cpp
	$(CXX) -c -o codec.o codec.cpp $(CFLAGS)

image.o: image.cpp
	$(CXX) -c -o image.o image.cpp $(CFLAGS)

.PHONY: clean
clean:
	rm *.o
