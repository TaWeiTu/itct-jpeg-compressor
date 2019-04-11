CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14

all: decode

decode: decode.cpp huffman.o codec.o
	$(CXX) $(CFLAGS) huffman.o decode.cpp -o decode

%: %.cpp
	$(CXX) -c $@.cpp -o $@.o $(CFLAGS)

.PHONY: clean
clean:
	rm *.o
