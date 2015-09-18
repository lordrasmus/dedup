
all:
	
	g++ -o dedup main.cpp -lssl -lcrypto  
	./dedup
