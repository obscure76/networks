#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>


int main(int argc, char** argv) 
{

	int socket_handle ;
	uint16_t port;
	struct sockaddr_in csd ;
	struct hostent* hret;
	char * input_buffer;
	char request[100];

	
	if(argc<4)
    {
        printf("\n usage: ./client <server> <port> <file> <host>\n");
        exit(0);
    }
    memset(request, 0, 100);
	input_buffer = malloc(20000);
	memset(input_buffer, 0, 20000);
    port = atoi(argv[2]);
	
	csd.sin_family = AF_INET;
	csd.sin_port = htons(port);

    hret = gethostbyname(argv[1]);
   	memcpy(&csd.sin_addr.s_addr, hret->h_addr, hret->h_length);
    sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", argv[3], argv[4]);

    printf("\n Sending request: \n%s\n", request);
	
	if((socket_handle = socket ( AF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("socket:");
        exit(1);
    }

	if (connect (socket_handle,(struct sockaddr*)&csd, sizeof ( struct sockaddr)) == -1 )
	{
		perror( "Couldnt connect to server\n" ) ;
        exit(1);
	}
	while(1)
    {
	if (send (socket_handle, request, 100, 0) <0)
		perror("send:");
        if(recv ( socket_handle , input_buffer , 20000, 0 ) >0)
            break;
    }
	printf ( "%s\n", input_buffer ) ;

	return 0 ;
}


