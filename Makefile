CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14
DBGFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14 -fsanitize=undefined -fsanitize=leak -fsanitize=address

all: decode

debug: decode.cpp codec.o huffman.o buffer.o
	$(CXX) -g huffman.o codec.o buffer.o decode.cpp -o decode $(DBGFLAGS)

decode: decode.cpp codec.cpp huffman.cpp buffer.cpp
	$(CXX) -c buffer.cpp -o buffer.o
	$(CXX) -c huffman.cpp -o huffman.o
	$(CXX) -c codec.cpp -o codec.o
	$(CXX) buffer.o huffman.o codec.o decode.cpp -o decode $(CFLAGS)

%: %.cpp
	$(CXX) -c -o $@.o $^ $(CFLAGS)

.PHONY: clean
clean:
	rm *.o
