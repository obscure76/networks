/*
 * common.h
 * All the types are defined in this
 */ 


#include<stdio.h>
#include<stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>



/*
 * SBCP MSG TYPES
 */ 
#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5
#define MAX_CLIENTS 10
/*
 * ERROR code TYPES
 */ 
#define FILENOTFOUND 1
#define NOT_VALID1 3

typedef enum
{
        false = ( 1 == 0 ),
            true = ( ! false )
} bool;



