COMPILER = g++
FLAGS = -Wall -Werror -pthread -std=c++14

.PHONY: clean all server client

all: server client

server: create bin/server
client: create bin/client

bin/server: src/server/main.cpp
	$(COMPILER) $^ $(FLAGS) -o $@
	
bin/client: src/client/main.cpp
	$(COMPILER) $^ $(FLAGS) -o $@

create:
	mkdir -p bin

clean:
	rm -rf bin/
