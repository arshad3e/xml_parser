all: xml_parser

lib: libpugixml.a

app: libpugixml.a Tcp_Server Tcp_Client

clean:
	rm -rf lib/libpugixml.a app/tcp_server app/tcp_client

xml_parser: libpugixml.a Tcp_Server Tcp_Client #libpugixml.a is dependency for server

pugixml: lib/pugixml.cpp 
	g++ -g -ggdb -c lib/pugixml.cpp -o lib/pugixml.o

libpugixml.a: lib/pugixml.o
	ar -rv lib/libpugixml.a lib/pugixml.o

Tcp_Server: lib/tcp_server.cpp
	g++ lib/tcp_server.cpp -L. lib/libpugixml.a -lpthread -o app/tcp_server

Tcp_Client: lib/tcp_client.cpp
	g++ lib/tcp_client.cpp -lpthread -o app/tcp_client
