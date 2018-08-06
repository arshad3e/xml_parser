#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
 
using namespace std;
bool keepRunning;

class tcp_client
{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;
     
public:
    tcp_client();
    ~tcp_client();
    bool conn(int);
    bool send_data();
    bool receive(int);
};
 
tcp_client::tcp_client()
{
    sock = -1;
    port = 0;
    address.assign("127.0.0.1");
}

tcp_client::~tcp_client()
{
    close(sock);
}

bool tcp_client::conn(int port)
{
    if(sock == -1)
    {
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            perror("Could not create socket");
            return false;
        }
         
        cout << "\033[1;33mSocket created\033[0m\n";
    }
    else    {   /* OK , nothing */  }

    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *he;
        struct in_addr **addr_list;

        if ( (he = gethostbyname( address.c_str() ) ) == NULL)
        {
            herror("gethostbyname");
           cout << "\033[1;31mFailed to resolve hostname\033[0m\n";
            return false;
        }
        addr_list = (struct in_addr **) he->h_addr_list;
 
        for(int i = 0; addr_list[i] != NULL; i++)
        {
            server.sin_addr = *addr_list[i];
            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;
            break;
        }
    }
    else
    {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }
     
    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        cout<<"Failed to connect " << address << ":"<<port << endl;
        return false;
    }
     
    cout<<"Connected to " << address << ":"<<port << endl;
    return true;
}
 
bool tcp_client::send_data()
{
    string filename;

    filename.clear();
    cout << "\033[1;32mEnter filename: \033[0m";
    //getline(cin, filename);
    cin >> filename;

    if(filename == "exit")
    {
		write(sock, filename.c_str(), strlen(filename.c_str()));
		keepRunning = false;
		return false;
    }
    write(sock, filename.c_str(), strlen(filename.c_str()));

    FILE *fs = fopen(filename.c_str(), "r");
    if(fs == NULL)
    {
       cout << "\033[1;31mERROR: File not found.\033[0m\n";
       return false;
    }

    fseek(fs, 0, SEEK_END);
    size_t size = ftell(fs);
    rewind(fs);
    char * buffer;

    buffer = (char*) malloc (sizeof(char)*size);
    if (buffer == NULL)
    {
       perror ("Memory error");
       return false;
    }

    size_t result = fread (buffer,sizeof(char),size,fs);
    if (result != size)
    {
       perror ("Reading error");
       return false;
    }
    if(send(sock, buffer, size, 0) < 0)
    {
       perror("Send failed : ");
       return false;
    }

    cout << "\033[1;32mData sent to server\033[0m\n";
    fclose(fs);
    return true;
}
 
bool tcp_client::receive(int size=512)
{
   char revbuf[size];
   string revbuffer;
   bzero(revbuf, size);
   int recvSize = 0;

    revbuffer.clear();
    //Receive a reply from the server
    while((recvSize = recv(sock , revbuf , sizeof(revbuf) , 0)) > 0)
    {
        revbuffer = revbuffer + revbuf;
        bzero(revbuf, size);
        if (recvSize == 0 || recvSize != 512)
        {
            break;
        }
    }
    if(recvSize < 0)
    {
        if (errno == EAGAIN)
        {
            cout << "\033[1;31mrecv() timed out.\033[0m\n";
            return false;
        }
        else
        {
           fprintf(stderr, "recv() failed due to errno = %d\n", errno);
           return false;
         }
    }
    cout << "\033[1;32m Data received from server:\033[0m\n" << revbuffer <<endl ;
    return true;
}

void exitProgram(int signum)
{
   // cleanup and close up stuff here  
   // terminate program  
    keepRunning = false;
    cout << "\033[1;34mClient exit program shutdown request\033[0m\n";
    exit(signum);  

}

int main(int argc , char *argv[])
{
   // register signal SIGINT and signal handler  
    signal(SIGINT, exitProgram); 

    tcp_client client;
    unsigned int portNo;

    cout << "\033[1;32mEnter port number : \033[0m";
    cin>>portNo;

    //connect to host
    if(client.conn(portNo) == true)
    {
		keepRunning = true;
		while(keepRunning)
		{
		   //send some data
		   if(client.send_data() == true)
		   {
		       client.receive();
		   }
		}
    }
    return 0;
}
