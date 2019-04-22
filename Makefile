CXX = g++-8
CFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++17
DBGFLAGS = -Wall -Wextra -Wconversion -O3 -std=c++17 -fsanitize=undefined -fsanitize=leak -fsanitize=address -DDEBUG

all: decode encode

debug: codec_dbg.o huffman_dbg.o buffer.hpp image_dbg.o decode.cpp encode.cpp
	$(CXX) image_dbg.o huffman_dbg.o codec_dbg.o decode.cpp -o decode $(DBGFLAGS)
	$(CXX) image_dbg.o huffman_dbg.o codec_dbg.o encode.cpp -o encode $(DBGFLAGS)

decode: codec.o huffman.o buffer.hpp parse.hpp image.o decode.cpp
	$(CXX) image.o huffman.o codec.o decode.cpp -o decode $(CFLAGS)

encode: codec.o huffman.o buffer.hpp parse.hpp image.o encode.cpp
	$(CXX) image.o huffman.o codec.o encode.cpp -o encode $(CFLAGS)

huffman.o: huffman.cpp huffman.hpp
	$(CXX) -c -o huffman.o huffman.cpp $(CFLAGS)

codec.o: codec.cpp codec.hpp
	$(CXX) -c -o codec.o codec.cpp $(CFLAGS)

image.o: image.cpp image.hpp
	$(CXX) -c -o image.o image.cpp $(CFLAGS)

huffman_dbg.o: huffman.cpp huffman.hpp
	$(CXX) -c -o huffman_dbg.o huffman.cpp $(DBGFLAGS)

codec_dbg.o: codec.cpp codec.hpp
	$(CXX) -c -o codec_dbg.o codec.cpp $(DBGFLAGS)

image_dbg.o: image.cpp image.hpp
	$(CXX) -c -o image_dbg.o image.cpp $(DBGFLAGS)

.PHONY: clean
clean:
	rm *.o
