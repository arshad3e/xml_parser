/*******************************************************************************
File name:
   tcp_server.cpp

Description:
Source code for tcp server
*******************************************************************************/

//INCLUDE's
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "pugixml.hpp"
#define LENGTH 512

using namespace std;
using namespace pugi;

bool keepRunning;
static int connFd;
class tcp_server
{
private:
    int pID;
    int listenFD;
    int noThread;
    int totalnoThreadSupported;
    socklen_t len;
    struct sockaddr_in svrAdd;
    struct sockaddr_in clntAdd;
    pthread_t threadA[255];

    // Helper functions to use pthreads
    static void* tcp_server_helper(void* tcpserver);
public:
    tcp_server();
    ~tcp_server();
    bool startServer(unsigned int);
    void recvFromClient();
};

/*******************************************************************************
Function name:
   tcp_server()

Description:
   Creates a tcp_server instance.

Parameter Description:
   None
*******************************************************************************/
tcp_server::tcp_server()
{
   pID =0;
   listenFD = -1;
   noThread = 0;
   //connFd = -1;
   totalnoThreadSupported = 255;
}

/*******************************************************************************
Function name:
   tcp_server()

Description:
   Cleans a tcp_server instance.

Parameter Description:
   None
*******************************************************************************/
tcp_server::~tcp_server()
{
    for(int i = 0; i < totalnoThreadSupported; i++)
    {
        pthread_join(threadA[i], NULL);
    }
    close(listenFD);
}

/*******************************************************************************
Function name:
   startServer

Description:
   Creates a server with port provided

Parameter Description:
   int = port to bind the socket 

Return Description:
   Return true if device added, false if not
*******************************************************************************/
bool tcp_server::startServer(unsigned int portNo)
{
   //create the socket
   listenFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

   if(listenFD < 0)
    {
        perror("Cannot open socket");
        return false;
    }
    
    bzero((char*) &svrAdd, sizeof(svrAdd));
    
    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_port = htons(portNo);
    
    //bind socket
    if(bind(listenFD, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0)
    {
        perror("Cannot bind");
        return false;
    }
    else
    {
        cout << "\033[1;33mBind successful\033[0m\n";
        listen(listenFD, 5);
        return true;
    }
}

void tcp_server::recvFromClient()
{
    keepRunning = true;
    while ((noThread < totalnoThreadSupported) && (keepRunning == true))
    {
        cout << "\033[1;32mListening\033[0m\n";
        len = sizeof(clntAdd);
        //this is where client connects. svr will hang in this mode until client conn
        connFd = accept(listenFD, (struct sockaddr *)&clntAdd, &len);

        if (connFd < 0)
        {
            perror("Cannot accept connection");
            continue;
        }
        else
        {
            cout << "\033[1;33mConnection successful\033[0m\n";
        }
        int *param = (int*) malloc( sizeof(connFd) );
        *param = connFd;
        pthread_create(&threadA[noThread], NULL, &tcp_server::tcp_server_helper,param);
        noThread++;
    }
    return;
}

void* tcp_server::tcp_server_helper(void * param)
{
    int *fdptr = (int*) param;
    char filename[LENGTH];
    char revbuf[LENGTH];
    int fr_block_sz;
    string buffer,FilePath;
    bool loop = true;
    FILE *fr;
    int newFD = *fdptr;
    free( fdptr );
    while(loop)
    {
       bzero(filename,LENGTH);
       buffer.clear();
       cout << "\033[1;32mwaiting for file to read from client\033[0m\n";

       int rv = read(newFD, filename, LENGTH);
       if((strcmp(filename,"exit") == 0) || (rv <= 0))
       {
          close(newFD);
          cout <<"\033[1;34m client exited\033[0m\n";
          loop = false;
          pthread_exit(NULL);
       }

       FilePath = filename;
       FilePath = "server_" + FilePath;

       fr = fopen(FilePath.c_str(), "w+");
       if(fr == NULL)
       {
            cout << "\033[1;31mFile Cannot be opened file on server\033[0m\n";
            buffer = "File Cannot be opened file on server\n";
            //send response to client
            if(send(newFD, buffer.c_str(), buffer.size(), 0) < 0)
            {
                perror("Send failed : ");
            }
            else
            {
                cout << "\033[1;33mData send to client\033[0m\n";
            }
            remove( FilePath.c_str()) ;  // Delete the received file
            continue;
        }

       bzero(revbuf, LENGTH);
       fr_block_sz = 0;
       while((fr_block_sz = recv(newFD, revbuf, LENGTH, 0)) > 0) 
       {
           int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
           if(write_sz < fr_block_sz)
           {
               perror("File write failed on server");
           }
           bzero(revbuf, LENGTH);
           if (fr_block_sz == 0 || fr_block_sz != 512) 
           {
               break;
           }
       }
       if(fr_block_sz < 0)
       {
           if (errno == EAGAIN)
           {
               printf("recv() timed out.\n");
               remove( FilePath.c_str());
               continue;
           }
           else
           {
               fprintf(stderr, "recv() failed due to errno = %d\n", errno);
               remove( FilePath.c_str());
               close(newFD);
               pthread_exit(NULL);
           }
       }
       cout << "\033[1;34mFile received from client\033[0m\n";
       fclose(fr);

       cout << "\033[1;37mFile parser started\033[0m\n";
       xml_document xmlDoc;
       xml_parse_result result = xmlDoc.load_file(FilePath.c_str());

       if (result.status != status_ok)
       {
            cout << "XML Parse error" << endl;
            buffer = "XML Parse error\n";
            //send response to client
            if(send(newFD, buffer.c_str(), buffer.size(), 0) < 0)
            {
                perror("Send failed : ");
            }
            else
            {
                cout << "\033[1;33mData send to client\033[0m\n";
            }
            remove( FilePath.c_str());
            continue;
       }

       //Parses the xml file provided and performs the command operation in it
       buffer.clear();
       xml_node commandNode = xmlDoc.child("Message").child("Command");
       if(commandNode == NULL)
       {
           cout << "\033[1;32mInvalid command\033[0m\n";
           buffer = "Invalid data format in received xml file\n";
           //send response to client
           if(send(newFD, buffer.c_str(), buffer.size(), 0) < 0)
           {
               perror("Send failed : ");
           }
           else
           {
               cout << "\033[1;33mData send to client\033[0m\n";
           }
           remove( FilePath.c_str());
           continue;
       }
       std::string command(commandNode.child_value());

       if((command.compare("Print")) == 0)
       {
          buffer = buffer + "Print command recieved \n" ;
          xml_node data = xmlDoc.child("Message").child("Data");
          for(xml_node row = data.first_child(); row ; row = row.next_sibling())
          {
             xml_node description = row.child("Description");
             xml_node value = row.child("Value");
             cout << description.child_value() << ": " << value.child_value() << endl;
             buffer  = buffer + description.child_value() + ": " + value.child_value() + '\n';
          }
       }
       else
       {
          cout << "\033[1;33mUnknown Command \033[0m\n";
          buffer  = buffer + "Unknown Command \n";
       }
       remove( FilePath.c_str()) ;  // Delete the received file

       //send response to client
       if(send(newFD, buffer.c_str(), buffer.size(), 0) < 0)
       {
           perror("Send failed : ");
       }
       else
       {
           cout << "\033[1;33mData send to client\033[0m\n";
       }
   }
   close(newFD);
   return NULL;
}

/*******************************************************************************
Function name:
   exitProgram

Description:
   Handles the signal to exit

Parameter Description:
   sig

Return Description:
   None
*******************************************************************************/
void exitProgram(int signum)
{
    keepRunning = false;
    cout << "\033[1;34mServer exit program shutdown request\033[0m\n";
    exit(signum);
}

int main(int argc , char *argv[])
{

   // register signal SIGINT and signal handler  
    signal(SIGINT, exitProgram);
    signal(SIGPIPE,SIG_IGN);

    tcp_server server;
    unsigned int portNo;

    cout << "\033[1;32mEnter port number : \033[0m";
    cin >> portNo;

    if(server.startServer(portNo) != true)
    {
        cout << "\033[1;31mStarting server failed\033[0m\n";
        return -1;
    }

    server.recvFromClient();

    return 0;
}
