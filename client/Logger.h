// Assignment 2: Embedded Debug Logging
// By: Serach Boes
// Student#: 115624181
// Due Date: Sunday, August 7th 2022

/* Header file for Logger.cpp */

#include <string> //needed for char * usage

using namespace std;

//The logger will type-define an enumeration called LOG_LEVEL in its header 
//file (Logger.h) specifying the log levels: DEBUG, WARNING, ERROR, CRITICAL.
enum LOG_LEVEL {
    DEBUG = -1,
    WARNING = 0,
    ERROR = 1,
    CRITICAL = 2
};

/*The logger will expose the following functions to each process via its header file (Logger.h):
1. int InitializeLog();
2. void SetLogLevel(LOG_LEVEL level);
3. void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message);
4. void ExitLog();*/

void InitializeLog();

void SetLogLevel(LOG_LEVEL level);

void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message);
                          
void ExitLog();

//declare thread running function
void run_thread(int fd);