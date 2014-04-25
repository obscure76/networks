/*
 * Server.c is responsible for server functionality
 * server adds a user and 
 */ 

#include<common.h>

char *user_array[30];
int user_count = 0;
int index_socket[20];

FILE *fptr;
char *FILE_NAME;
uint16_t cport;

int read_msg(char *buf)
{
	/* 				Parse RRQ 
	
	              2B      string      1B      string      1B
		     -----------------------------------------------
	      RRQ  |  01  |  Filename  |   0  |    Mode    |   0  |
		     -----------------------------------------------
	 */
	uint16_t opcode, offset=0, len, *temp;
	char filename[20], mode[10];
	int i;
	
	temp = (uint16_t *)buf;
	//printf("\n opcode rev %d", *temp);
	(*temp)=(*temp)>>8;
	opcode = *temp;
	
	offset+=2;
	//	printf("\n opcode %d\n", opcode);

	if (opcode!=RRQ) 
	{
		printf("\n Not a read request: Close the conn\n");
		return 2;
	}

	len = strlen(buf+offset);

	strncpy(filename, buf+offset, len);
	for(i=0; i<=len;i++)
    {
        if(!(isalpha(*(filename+i))))
        {
            *(filename+i) = '\0';
            len = i+1;
            break;
        }
    }
    printf("\n len = %d", i);
    strncpy(FILE_NAME, filename, len);
	offset+=(len+2);
	//printf("\n the received filename is opcode %d %s", opcode, FILE_NAME);
	
	len = strlen(buf+offset);
	strncpy(mode, buf+offset, len); 
	
	printf("\n the RRQ has been processed ");
	return 1;
}

int read_ack(char *buf, uint16_t *block)
{
	uint16_t opcode, offset=0, num;
	uint8_t *temp;
	if (!buf)
	{
		*block=0;
		return 0;
	}
    temp = (uint8_t *)(buf+1);
    opcode = *temp;
    //opcode<<8;
    temp = (uint8_t *)buf;
    opcode |= (uint16_t)(*temp);
	offset+=2;
   
    if (opcode==1)
        return 2;
    else if(opcode!=4)
        return 1;
    temp = (uint8_t *)(buf+3);
    num = *temp;
    temp = (uint8_t *)(buf+2);
    num |= (uint16_t)(*temp);
    *block=num;
    //printf("\n the ack opcode is %d  block# %d", opcode, num);
    //printf("\n the ack opcode is %d  block# %d", opcode, num);
	return 0;
}

void child_handler(int s) {
    while (wait( NULL) > 0)
        ; /* wait for any child to finish */
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv)
{
    int ssd, read;
    struct sockaddr_in addr, src_addr;
    socklen_t addrlen;
    struct hostent* hret;
    int  n_bytes;
    uint16_t port;
    char buf[50];
    char join[600];
    uint16_t frame_count =0, ack_count = 0, data=3, error=5, errcode =1;
    char *errmsg= "not found\0";
    uint8_t *temp;
    struct sigaction sa;
    pid_t pid;

    FILE_NAME = (char *)malloc(20); 
    cport = 4000;
    if(argc<2)
    {
	  printf("\n Usage : ./server <localhost> <port> \n");
	  exit(1);
    }
    sa.sa_handler = child_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL)==-1)
    {
        perror("Sigaction");
        exit(1);
    }


    if ((ssd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
    {
	    perror("\n Cannot create server socket");
	    exit(1);
    }
    
    port = atoi(argv[2]); 
    hret = gethostbyname(argv[1]);
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);


    if(bind(ssd, (struct sockaddr*)&addr, sizeof(struct sockaddr))==-1)
    {
	    perror("\n Failed to bind");
	    exit(1);
    }
    addrlen = sizeof(struct sockaddr);

    printf("\n listening on port%d\n", port);
    while(1)
    {
	    memset(buf, 0, sizeof(buf));
	    n_bytes = recvfrom(ssd, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &addrlen);
        //printf("\n recvd %s %d", buf, n_bytes);
	    /* process the received message */
	    if (n_bytes > 0) {
	        pid = fork();
	        if (pid == -1)//error
            {
                continue;
            } else if (pid > 0)//parent
            {
                continue;
            } else {//child
                close(ssd);
                if ((ssd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
                {
        	        perror("\n Cannot create server socket");
	                exit(1);
                }
                hret = gethostbyname(argv[1]);
                //bzero((char *)&addr, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(cport);
                memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);
                cport+=2;
                if(bind(ssd, (struct sockaddr*)&addr, sizeof(struct sockaddr))==-1)
                {
            	    perror("\n Failed to bind");
            	    exit(1);
                }
	            if(read_msg(buf) ==1)
                {
                printf("\n file %s", FILE_NAME);
    			fptr = fopen(FILE_NAME, "r");
    			if (!fptr)
                {
                    printf("\n File not found");
				    temp = (uint8_t *)&error;
    				memcpy(join, temp+1, 1);
    				memcpy(join+1, temp, 1);
				    temp = (uint8_t *)&errcode;
    				memcpy(join+2, temp+1, 1);
	    			memcpy(join+3, temp, 1);
	    			memcpy(join+4, errmsg, strlen(errmsg));
	    			n_bytes = 4+ strlen(errmsg);
				    if(sendto(ssd, join, n_bytes, 0, (struct sockaddr*)&src_addr, \
							sizeof(struct sockaddr)) == -1)
				    {
					    perror("\n Sendto:");
				        continue;
                    }
                }
			    while(!feof(fptr) && !ferror(fptr))
	    		{
				/* Form a data packet */
				frame_count++;
				temp = (uint8_t *)&data;
				memcpy(join, temp+1, 1);
				memcpy(join+1, temp, 1);
				temp = (uint8_t *)&frame_count;
				memcpy(join+2, temp+1, 1);
				memcpy(join+3, temp, 1);

				//printf("\n  file curr %p", fptr);
				n_bytes = fread(join+4, 1, 512, fptr);
				n_bytes+=4;
				printf("\n  to receiver frame# %d", frame_count);
				if(sendto(ssd, join, n_bytes, 0, (struct sockaddr*)&src_addr, \
							sizeof(struct sockaddr)) == -1)
				{
					perror("\n Sendto:");
					break;
				}
				memset(buf, 0, 50);
				memset(join, 0, 520);
	    		n_bytes = recvfrom(ssd, (char *)buf, 50, 0, \
						(struct sockaddr*)&src_addr, &addrlen);
				if (n_bytes<=0)
				{
					perror("\n No Ack from receiver ");
					continue;
				}
				read = read_ack(buf, &ack_count);
				if (read==1)
				    break;
			    }
	            fclose(fptr);
	            frame_count=0;
        	    memset(FILE_NAME, 0, 20);
	            }// End of request process 
	            close(ssd);
	            exit(0);
            }//End of child
        } else {
            perror("\n recvfrom:");
            continue;
        }
    }

    printf("\n port %d", port);
    return 0;
}

