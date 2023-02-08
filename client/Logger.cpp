// Assignment 2: Embedded Debug Logging
// By: Serach Boes
// Student#: 115624181
// Due Date: Sunday, August 7th 2022

/*
Program Description:
This code has an embedded logging mechanism with logs being sent to a central server which can keep track of how the code is performing.
Logger.cpp -> implements client functions for logging
*/

#include <sys/types.h>  //micellanious functions
#include <sys/socket.h> //defines the type socklen_t and other macros for socket use
#include <arpa/inet.h>  //makes available the type in_port_t and the type in_addr_t as defined in the description of <netinet/in.h>
#include <netinet/in.h> //needed with arpa/inet.h
#include <unistd.h>     //provides access to the POSIX operating system API
#include <stdio.h>      //standard Input Output, input/output function information
#include <stdlib.h>     //standard library of C, process control
#include <iostream>     //needed for user input, input/output
#include <string.h>     //for string functionality
#include <thread>       //for thread usage
#include <pthread.h>    //for pthread usage
#include <mutex>        //A Mutex is a lock that we set before using a shared resource and release after using it.

#include "Logger.h" //include header file for Logger.cpp

using namespace std;

// set buffer
const int BUF_LEN = 256;
char buf[BUF_LEN];

// variables needed for server connection
struct sockaddr_in server_addr;
int sock_fd;
string ip_address = "192.168.230.128";
const int port = 1153;
socklen_t socket_len;

LOG_LEVEL global = ERROR; // for logging
bool is_running = true;   // thread
mutex log_mutex;          // set mutex lock variable

// implement interface functions for logging

void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message)
{
    // compare the severity of the log to the filter log severity. The log will be thrown away if its severity is lower than the filter log severity.
    if (level > global)
    {

        // create a timestamp to be added to the log message. Code for creating the log message will look something like:
        time_t now = time(0);
        char *dt = ctime(&now);
        memset(buf, 0, BUF_LEN);
        char levelStr[][16] = {"DEBUG", "WARNING", "ERROR", "CRITICAL"};
        int len = sprintf(buf, "%s %s %s:%s:%d %s\n", dt, levelStr[level], prog, func, line, message) + 1;
        buf[len - 1] = '\0';

        // The message will be sent to the server via UDP sendto().
        sendto(sock_fd, buf, len, 0, (struct sockaddr *)&server_addr, socket_len);
    }
}

void InitializeLog()
{
    // create a non-blocking socket for UDP communications (AF_INET, SOCK_DGRAM).
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("ERROR: Unable to create socket");
        exit(EXIT_FAILURE);
    }

    // Set the address and port of the server.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());
    server_addr.sin_port = htons(port);

    socket_len = sizeof(server_addr);

    // Start the receive thread and pass the file descriptor to it.
    thread receive_thread(run_thread, sock_fd);
    receive_thread.detach();
}

// will set the filter log level and store in a variable global
void SetLogLevel(LOG_LEVEL level)
{
    global = level;
}

// ExitLog() will stop the receive thread via an is_running flag and close the file descriptor.
void ExitLog()
{
    is_running = false; // stop the receive thread via an is_running flag
    close(sock_fd);     // close the file descriptor
}

// running receive thread
void run_thread(int fd) // accept the file descriptor as an argument.
{
    // Set 1 second timeout for the socket
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    // run in an endless loop via an is_running flag.
    while (is_running)
    {

        memset(buf, 0, BUF_LEN);

        // apply mutexing to any shared resources used within the recvfrom() function.
        log_mutex.lock();
        int size = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr *)&server_addr, &socket_len);

        string message = buf;

        // act on the command “Set Log Level=<level>” from the server to overwrite the filter log severity.
        if (size > 0)
        {
            string log_level = message.substr((message.find("=") + 1)); // find log level in message

          if(log_level == "DEBUG"){
                global = DEBUG;
            }else if(log_level == "WARNING"){
                global = WARNING;
            }else if(log_level == "ERROR"){
                global = ERROR;
            }else if(log_level == "CRITICAL"){
                global = CRITICAL;
            }else{
                continue;
            }
        }else{
            sleep(1);// ensure the recvfrom() function is non-blocking with a sleep of 1 second if nothing is received.
        }
       
        message = "";       // reset message variable to empty
        log_mutex.unlock(); // unlock mutex
    }
}
