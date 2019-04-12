CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14
DBGFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14 -fsanitize=undefined -fsanitize=leak -fsanitize=address

all: decode

debug: decode.cpp codec.o huffman.o buffer.o
	$(CXX) -g huffman.o codec.o buffer.o decode.cpp -o decode $(DBGFLAGS)

decode: codec.o huffman.o buffer.hpp
	$(CXX) huffman.o codec.o decode.cpp -o decode $(CFLAGS)

huffman.o: huffman.cpp
	$(CXX) -c -o huffman.o huffman.cpp $(CFLAGS)

codec.o: codec.cpp
	$(CXX) -c -o codec.o codec.cpp $(CFLAGS)

.PHONY: clean
clean:
	rm *.o
