
all:

	sudo rm -f /root/.config/dedub.cfg
	g++ -Wall -Wno-attributes -ggdb3 -std=c++11 -o dedup MurmurHash3.cpp main.cpp ui.cpp file_based_memory.cpp -lssl -lcrypto -lconfig++ -lcurses -lform
#	clang++ -std=c++11 -O3 -o dedup MurmurHash3.cpp main.cpp file_based_memory.cpp -lssl -lcrypto
#	@./dedup /usr/share/
	@sudo ./dedup .git
