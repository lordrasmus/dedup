
all:
	
	g++ -std=c++11 -o dedup main.cpp file_based_memory.cpp -lssl -lcrypto  
	./dedup /mnt/btrfs/
