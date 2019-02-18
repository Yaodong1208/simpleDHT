CC := g++
MPICXX := mpicxx
CFLAGS  := -g -Wall

dht: server client

server: common.h server.h server.cc
	$(MPICXX) $(CFLAGS) -std=c++17 -o server server.cc -lpthread

client: common.h client.h client.cc
	$(MPICXX) $(CFLAGS) -std=c++17 -o client client.cc -lpthread

clean:
	rm client
	rm server
