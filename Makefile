
all:

#	g++ -std=c++11 -o dedup main.cpp file_based_memory.cpp -lssl -lcrypto
	clang++ -std=c++11 -O3 -o dedup MurmurHash3.cpp main.cpp file_based_memory.cpp -lssl -lcrypto
#	@./dedup /usr/share/
	@./dedup .git
