CXXFLAGS = -std=c++2a -Wall -Werror
LDFLAGS = -pthread

all: client server

client: client.o
	$(CXX) $(LDFLAGS) $^ -o client.exe

server: server.o http.o
	$(CXX) $(LDFLAGS) $^ -o server.exe

http.o: http.h properties.h
client.o: properties.h
server.o: properties.h http.h

clean:
	rm *.o *.exe
