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
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/time.h>


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

#define fhttp = "HTTP"
#define fversion = "1.1"
#define frcode = "200 OK"
#define fdate = "Date:"
#define fserver = "Server:"
#define fmodified = "Last-Modified:"
#define faccept = "Accept-Ranges:"
#define flength = "Content-Length:"
#define fcache = "Cache-Control:"
#define fexpire = "Expires:"
#define fvary = "Vary:"
#define fconn = "Connection:"
#define ftype = "Content-Type:"
#define fetag = "ETag:"





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

typedef struct cache_
{
    char filename[20];
    char last_modified[50];
    char expiry[50];
    char etag[50];
    uint8_t rank;
} cache;
