CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++14

codec: codec.cpp ../codec.cpp ../huffman.cpp 
	$(CXX) -c ../codec.cpp -o codec.o $(CFLAGS)
	$(CXX) -c ../huffman.cpp -o huffman.o $(CFLAGS)
	$(CXX) $(CFLAGS) codec.o huffman.o $@.cpp

decode: decode.cpp huffman.o codec.o
	$(CXX) $(CFLAGS) huffman.o decode.cpp -o decode

.PHONY: clean
clean:
	rm *.o
