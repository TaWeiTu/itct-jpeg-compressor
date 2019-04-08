CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14

decode: decode.cpp huffman.o
	$(CXX) $(CFLAGS) huffman.o decode.cpp -o decode

%: %.cpp
	$(CXX) $(CFLAGS) -c $@.cpp -o $@.o

.PHONY: clean
clean:
	rm *.o
