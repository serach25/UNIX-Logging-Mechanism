// Assignment 2: Embedded Debug Logging
// By: Serach Boes
// Student#: 115624181
// Due Date: Sunday, August 7th 2022

/*
Program Description:
This code has an embedded logging mechanism with logs being sent to a central server which can keep track of how the code is performing.
LogServer.cpp -> server for logging
*/

#include <sys/types.h> //micellanious functions
#include <sys/socket.h> //defines the type socklen_t and other macros for socket use
#include <arpa/inet.h> //makes available the type in_port_t and the type in_addr_t as defined in the description of <netinet/in.h>
#include <netinet/in.h> //needed with arpa/inet.h
#include <unistd.h> //provides access to the POSIX operating system API
#include <iostream> //needed for user input, input/output
#include <string.h> //for string functionality
#include <pthread.h> //for pthread usage
#include <fcntl.h> //for file descriptors
#include <net/if.h> //header file contains network interface structures and definitions
#include <signal.h> //signal handling
#include <sys/un.h> //contains definitions for UNIX-domain sockets


using namespace std;

//variables needed for server client communication
const int PORT=1153; //port number to connect to
const char IP_ADDR[]="192.168.230.128"; //IP address to connect to
const char logFile[]="logFile.txt"; //create log file
const int BUF_LEN=4096; //length of buffer
char buf[BUF_LEN]; //create buffer
bool is_running; //is_running flag

//for socket connections
struct sockaddr_in remaddr; 
socklen_t addrlen;

pthread_mutex_t lock_x; //mutex lock

void *recvThread(void *arg); 

//The server will shutdown gracefully via a ctrl-C via a shutdownHandler
static void shutdownHandler(int sig)
{
    switch(sig) {
        case SIGINT:
            is_running=false;
            break;
    }
}

//main function
int main(void)
{
    int fd, ret, len;
    struct sockaddr_in myaddr;
    addrlen = sizeof(remaddr);

    //for graceful shutdown
    signal(SIGINT, shutdownHandler);

    ////The server’s main() function will create a non-blocking socket for UDP communications (AF_INET, SOCK_DGRAM).
    fd=socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(fd<0) {
        //print error message if socket creation fails and exit program with -1 exit code
        cout<<"Cannot create the socket"<<endl;
        cout<<strerror(errno)<<endl;
        return -1;
    }

    //connect to port and ip address
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    ret = inet_pton(AF_INET, IP_ADDR, &myaddr.sin_addr);
    if(ret==0) {
        //if fails print out error message and exit program with -1 exit code
        cout<<"No such address"<<endl;
        cout<<strerror(errno)<<endl;
        close(fd);
        return -1;
    }
    myaddr.sin_port = htons(PORT);

    //The server’s main() function will bind the socket to its IP address and to an available port, 1153
    ret = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
    if(ret<0) {
        //if fails to bind the socket print out error message and exit program with exit code -1
        cout<<"Cannot bind the socket to the local address"<<endl;
        cout<<strerror(errno)<<endl;
        return -1;
    }

    // The server’s main() function will create a mutex and apply mutexing to any shared resources.
    pthread_mutex_init(&lock_x, NULL);

    //The server’s main() function will start a receive thread and pass the file descriptor to it.
    pthread_t recvId;
    ret = pthread_create(&recvId, NULL, recvThread, &fd); //call recvThread function for thread run

    //The server’s main() function will present the user with three options via a user menu:
    int selection=-1;
    while(selection!=0) { //i.e. while the user has not requested a shut down
        system("clear"); //clear screen
        //print menu
        cout<<" 1. Set the log level"<<endl;
        cout<<" 2. Dump the log file here"<<endl;
        cout<<" 0. Shut down"<<endl;
        cin>>selection; //enter user selection
        //if the user selection is valid, i.e. one of the menu options
	if(selection>=0 && selection<=2) {
            cout<<endl;
	    int level, fdIn, numRead;
	    char key;
	    switch(selection) {
                case 1:  /*setting the log level*/
		    cout<<"What level? (0-Debug, 1-Warning, 2-Error, 3-Critical):"; //The user will be prompted to enter the filter log severity.
		    cin>>level;
		    if(level<0 || level>3) cout<<"Incorrect level"<<endl;
		    else {
                //The information will be sent to the logger
                        pthread_mutex_lock(&lock_x);
                        memset(buf, 0, BUF_LEN);
                        len=sprintf(buf, "Set Log Level=%d", level)+1;
                        sendto(fd, buf, len, 0, (struct sockaddr *)&remaddr, addrlen); 
                        pthread_mutex_unlock(&lock_x);
		    }
                    break;
                case 2: /*dumping log files to the screen*/
               
		    fdIn=open(logFile, O_RDONLY); //The server will open its server log file for read only.
		    numRead=0;
                    pthread_mutex_lock(&lock_x); //lock mutex
		    do {
                        numRead = read(fdIn, buf, BUF_LEN); //It will read the server’s log file contents and display them on the screen.
			cout<<buf;
		    } while (numRead>0);
                    pthread_mutex_unlock(&lock_x); //unlock mutex
		    close(fdIn); //close file descriptor when finished
            //On completion, it will prompt the user with: "Press any key to continue:"
		    cout<<endl<<"Press any key to continue: ";
                    cin>>key;
                    break;
                case 0: /*shutting down*/
		    is_running=false; //The receive thread will be shutdown via an is_running flag.
                    break;
	    }
	}
    }
    //The server will exit its user menu and The server will join the receive thread to itself so it doesn’t shut down before the receive thread does
    pthread_join(recvId, NULL);
    close(fd); //close file descriptor when done
    return 0; //end program, return zero
}

//The server’s receive thread
void *recvThread(void *arg)
{
    int *fd = (int *)arg;

    int openFlags = O_CREAT | O_WRONLY | O_TRUNC; //open the server log file for write only with permissions rw-rw-rw-
    mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //equivalent to rw-rw-rw- permissions
    int fdOut=open(logFile, openFlags, filePerms);//open file

    int len;
    is_running=true;
    while(is_running) { //run in an endless while loop via an is_running flag.
    //apply mutexing to any shared resources used within the recvfrom() function
        pthread_mutex_lock(&lock_x); //lock mutex before accessing
        len = recvfrom(*fd, buf, BUF_LEN, 0, (struct sockaddr *)&remaddr, &addrlen)-1;
	if(len<0) {
            pthread_mutex_unlock(&lock_x); //apply mutexing to any shared resources used within the recvfrom() function. - unlock mutex
            sleep(1); //ensure the recvfrom() function is non-blocking with a sleep of 1 second if nothing is received.
	} else {
            write(fdOut, buf, len); //take any content from recvfrom() and write to the server log file.
            pthread_mutex_unlock(&lock_x); //unlock mutex
	}
    }

    cout<<"pthread_exit"<<endl;
    pthread_exit(NULL); //exit thread
}