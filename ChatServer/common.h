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
#include <unistd.h>


#define VERSION 3

#define FAIL -1
#define SUCCESS 0

/*
 * SBCP MSG TYPES
 */ 
#define JOIN 2
#define FWD 3
#define SEND 4

/*
 * SBCP ATTR TYPES
 */ 
#define RSN_FAIL 1
#define USERNAME 2
#define CLIENT_COUNT 3
#define MSG 4

typedef enum
{
        false = ( 1 == 0 ),
            true = ( ! false )
} bool;

//bool ver[9] = {'0', '0', '0', '0', '0', '0', '0', '1', '1'};

typedef struct my_packet_
{
    uint16_t vertype;
    uint16_t length;
    uint16_t attrtype;
    uint16_t attrlen;
    char buf[512];
    char *sam;
} my_packet;


