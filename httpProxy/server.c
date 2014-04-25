/*
 * Server.c is responsible for server functionality
 * server adds a user and 
 */ 

#include<common.h>


cache *cache_array[10];
int read_msg(char *, char *, char *, char *, char *, char *);

int check_cache(char *filename)
{
    int i, len;
    if (!filename)
        return 0;
    len = strlen(filename);
    for (i = 0; i< 10; i++)
    {
        if(cache_array[i])
        {
            if(strncmp(filename, cache_array[i]->filename, len)==0)
            {
                /* The requested already in cache :send the contents*/
                return i;
            }
        }
    }
    return -1;
}

int display_cache(void)
{
	int i;
    for (i = 0; i< 10; i++)
    {
        if(cache_array[i])
        {
		printf("\n The file %s    rank %d",cache_array[i]->filename, cache_array[i]->rank);
        }
    }
    return 0;

}

int check_expiry(int cache_index, char *host, char *texpiry)
{
    char *tfile, *tlast_modified, *tetag;
    int wlistener, check;
    struct hostent *whret;
    struct sockaddr_in waddr;
    struct in_addr winaddr;
    char request[200];
    int nbytes, wport = 80;
    char buf[10000];
    FILE *fptr;
    
    
    tfile = cache_array[cache_index]->filename;
    tlast_modified = cache_array[cache_index]->last_modified;
    tetag = cache_array[cache_index]->etag;

    
    waddr.sin_family = AF_INET;
    waddr.sin_port = htons(wport);

    if (isdigit(*host))
    {
        if(!inet_aton(host, &winaddr))
        {
            perror("\n inet_aton:");
            return 1;
        }
        memcpy(&waddr.sin_addr.s_addr, &winaddr, sizeof winaddr);
    } else {
        whret = gethostbyname(host);
        if (!whret) {
            perror("gethostbyname:");
            return 1;
        }
        memcpy(&waddr.sin_addr.s_addr, whret->h_addr, whret->h_length);
    }
    if((wlistener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot bind socket");
        exit(1);
    }

    if (connect(wlistener, (struct sockaddr*)&waddr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    memset(request, 0, 200);
    sprintf(request, "GET /%s HTTP/1.0\r\nHost: %sIf-Modified-Since: %s"
            "If-None-Match: %s\r\n\r\n", tfile, host, tlast_modified, tetag);
    nbytes=strlen(request);
    printf("\n Sending: \n %s", request);
    if(send(wlistener, request, nbytes, 0) <=0)
    {
        perror("send:");
        exit(1);
    }
    memset(buf, 0, 10000);
    if ((nbytes = recv(wlistener, buf, 10000, 0)) <= 0)
    {
        perror("\n Nothing received");
        return 1;
    }
    //printf("\n Received: \n %s", buf);

    tfile = malloc(50);
    memset(tfile, 0 , 50);
    tlast_modified = malloc(50);
    memset(tlast_modified, 0 , 50);
    host = malloc(50);
    memset(host, 0 , 50);
    tetag = malloc(50);
    memset(tetag, 0 , 50);
    check = read_msg(buf, tfile, tlast_modified, texpiry, host, tetag);
    if (check == 3)
    {
        /* NOT MODIFIED */
        close(wlistener);
        return 2;
        
    } else if (check ==2)
    {
	/* File modified*/
	printf("\n file modified ");
        fptr = fopen(cache_array[cache_index]->filename, "w+");
        if(!fptr)
            perror("\n fopen:");
        fwrite(buf, strlen(buf), 1, fptr);
        fclose(fptr);
	strncpy(cache_array[cache_index]->last_modified, tlast_modified, 50);    
        strncpy(cache_array[cache_index]->expiry, texpiry, 50);    
        strncpy(cache_array[cache_index]->etag, tetag, 50);    

    }
    close(wlistener);

    return 0;
}

int send_response(int csd, int cache_index)
{
    FILE *fptr;
    char buf[10000];
    int j=0;

    memset(buf, 0, 10000);
    fptr = fopen(cache_array[cache_index]->filename, "r");
    if(fptr == NULL)
    {
        printf("\n Open a cache element failed");
        return 1;
    }
    while(!feof(fptr) && !ferror(fptr))
    {
        fread (buf+j,512,1,fptr);
        j+=512;
    }
    //printf("\n \n\n The file content: \n %s\n\n", buf); 
    if(strlen(buf)>0)
    {
        if(send(csd, buf, j, 0)==-1)
        {
            perror("\n send:");
            return 1;
        }
    }
    return 0;
}

int update_cache(int cache_index)
{
    int trank, i; 
    trank = cache_array[cache_index]->rank;
    
    if(trank ==1)
        return 0;
    cache_array[cache_index]->rank=1;
    for (i=0;i<10;i++)
    {
        if(cache_array[i] && i!=cache_index)
        {
            if(cache_array[i]->rank < trank)
                    cache_array[i]->rank+=1;
        }
    }
    return 0;
}

int insert_in_cache(char *tfile, char *tlast_modified, char *texpiry, char *tetag)
{
    int i,rank = 0, index=0;
    for (i=0;i<10;i++)
    {
        if(!cache_array[i])
            break;
    }
    if(i<10)
    {
        printf("\n Found the index %d",i);
        cache_array[i] = (cache *)malloc(sizeof (struct cache_));
        strncpy(cache_array[i]->filename, tfile, 20);    
        strncpy(cache_array[i]->last_modified, tlast_modified, 50);    
        strncpy(cache_array[i]->expiry, texpiry, 50);    
        strncpy(cache_array[i]->etag, tetag, 50);    
        cache_array[i]->rank = 0;
        index =i;
        for(i=0;i<10;i++)
        {
            if(cache_array[i])
                cache_array[i]->rank+=1;
        }
        return index;
    } else {
        /* Need to replace some entry with a high rank */
        for(i=0;i<10;i++)
        {
            if(cache_array[i]->rank > rank)
            {
                rank = cache_array[i]->rank;
                index = i;
            }
        }
        printf("\n Need to replace some cache entry %d", index);
        strncpy(cache_array[index]->filename, tfile, 20);    
        strncpy(cache_array[index]->last_modified, tlast_modified, 20);    
        strncpy(cache_array[index]->expiry, texpiry, 20);    
        cache_array[index]->rank = 0;
        for(i=0;i<10;i++)
        {
            if(cache_array[i])
                cache_array[i]->rank+=1;
        }
        return index;
    }
    return -1;
}


int read_msg(char *buf, char *file, char *last_modified, char *expires, char *host, char *etag)
{
    char str[50];
    int i=0, j =0, ret_val=0;
    FILE *fptr;
    char *OK = "200 OK";
    char *NMF = "304 Not Modified";


    if(!buf)
    {   
        printf("\n Read message fail");
        return 0;
    }
    fptr = fopen("simple", "w+");
    if (!fptr) {
        printf("\n1 Could not open a file");
        exit(0);
    }
    fwrite(buf, strlen(buf), 1, fptr);
    rewind(fptr);
    while(!feof(fptr) && !ferror(fptr))
    {
        fgets(str, 50, fptr);
        if (strlen(str) >=1)
        {
            switch(str[0])
            {
                case 'G':
                    if(str[i+1] == 'E' && str[i+2] == 'T')
                    {
                        while(str[i++] != ' ');i++;
                        while(str[i] !=' ')
                        {
                            *(file+j) = *(str+i);
                            i++;
                            j++;
                        }
                        *(file+j)='\0';
                        printf("\n File: %s requested\n", file);
                    }
                    i=0;
                    break;
                case 'H':
                    if (str[1] == 'T' || str[1] == 't') 
                    {
                        while(str[i++] != ' ');
                        if (strncmp(OK, str+i, strlen(OK)) == 0)
                        {
                            printf("\n HTTP 200  OK response ");
                            ret_val =2;
                        } else if (strncmp(NMF, str+i, strlen(NMF)) ==0){
                        printf("\n\n HTTP 304 NOT modified");
                            ret_val =3;
                        } else {
                            printf("\n Error %s", str);
                        }
                    } else if (str[1] == 'o' || str[1] == 'O') {
                        while(str[i++] != ' ');
                        strncpy(host, str+i, strlen(str+i));
                        //printf("\n Host %s", host);
                        ret_val =1;
                    }
                    i=0;
                    break;
                case 'L':
                    while(str[i++] != ' ');
                    strncpy(last_modified, (str+i), strlen(str+i));
                    //printf("\nLast modified :%s\n", last_modified);
                    i=0;
                    break;
                case 'E':
                    if(str[1] == 'x')
                    {
                        while(str[i++] != ' ');
                        strncpy(expires, (str+i), strlen(str+i));
                        //printf("\nExpiry :%s\n", expires);
                    } else if(str[1] == 'T')
                    {
                        while(str[i++] != ' ');
                        strncpy(etag, (str+i), strlen(str+i));
                        printf("\nEtag :%s\n", etag);
                    }
                    i=0;
                    break;
            }
        }
    }
    fclose(fptr); 
    printf("\nreceived message processed\n");
    return ret_val; 
}

int get_file_from_server(char *host, char *tfile, char *tlast_modified, char *texpiry, char *tetag)
{
    int wlistener;
    struct hostent *whret;
    struct sockaddr_in waddr;
    struct in_addr winaddr;
    char request[200];
    int nbytes, wport = 80;
    char buf[10000];
    FILE *fptr;
    
    
    waddr.sin_family = AF_INET;
    waddr.sin_port = htons(wport);

    if (isdigit(*host))
    {
        if(!inet_aton(host, &winaddr))
        {
            perror("\n inet_aton:");
            return -1;
        }
        //printf("\n Adress %s processed\n", host);
        memcpy(&waddr.sin_addr.s_addr, &winaddr, sizeof winaddr);
    } else {
        whret = gethostbyname(host);
        if (!whret) {
            perror("gethostbyname:");
            return -1;
        }
        memcpy(&waddr.sin_addr.s_addr, whret->h_addr, whret->h_length);
    }

    sprintf(request, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", tfile, host);
    printf("\n Sending req \n%s \n GET to server:", request);
    nbytes=strlen(request);
    
    if((wlistener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot bind socket");
        exit(1);
    }

    if (connect(wlistener, (struct sockaddr*)&waddr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }
    
    send(wlistener, request, nbytes, 0);
    memset(buf, 0, 10000);
    if ((nbytes = recv(wlistener, buf, 10000, 0)) <= 0)
    {
        perror("\n Nothing received");
        return -1;
    }
    if (read_msg(buf, tfile, tlast_modified, texpiry, host, tetag) == 2)
    {
        /* Got 200 OK from server : create a cache entry*/
        fptr = fopen(tfile, "w+");
        if(!fptr)
            perror("\n fopen:");
        fwrite(buf, strlen(buf), 1, fptr);
        fclose(fptr);
        printf("\n Local cache entry created\n");
    } else {
	    printf("\n File not present");
	    close(wlistener);
	    return -1;
    }
    close(wlistener);
    
    return 0;
}

void get_filename(char *string, char *file_name)
{
    int i=0,j=0;
    //printf("\n input string %s", string);
    while(*(string+j) != '\0')
    {
        if(isalpha(*(string+j)) || isdigit(*(string+j)))
        {
            *(file_name+i) = *(string+j);
            i++;
        }
        j++;
        //printf("\t %c", *(string+j));
    }
    *(file_name+i)='\0';
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
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int i=0, j=0, cache_index;
    struct sockaddr_in addr;
    uint16_t port;
    uint8_t MAX_USERS;
    struct hostent *hret;
    char request[100];
    char *tfile, *tlast_modified, *texpiry, *host, *tetag;

    memset(request, 0, 100);
    tfile = malloc(50);
    memset(tfile, 0 , 50);
    tlast_modified = malloc(50);
    memset(tlast_modified, 0 , 50);
    texpiry = malloc(50);
    memset(texpiry, 0 , 50);
    host = malloc(50);
    memset(host, 0 , 50);
    tetag = malloc(50);
    memset(tetag, 0 , 50);
    
    if(argc<3)
    {
        printf("\n Usage : ./server <localhost> <port> <clients>\n Exiting\n");
        exit(1);
    }
    
    
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    MAX_USERS = atoi(argv[3]);

    for (j=0; j<10; j++)
    {
        cache_array[j] = NULL;
    }
    port = atoi(argv[2]); 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    hret = gethostbyname(argv[1]);
    memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);

    
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot bind socket");
        exit(1);
    }
    if(bind(listener, (struct sockaddr*)&addr, sizeof(struct sockaddr))==-1)
    {
        perror("CCannot bind socket");
        exit(1);
    }
   

    if (listen(listener, 10) == -1) 
    {
        perror("listen");
        exit(3);
    }
    FD_SET(listener, &master);
    fdmax = listener; // so far, it's this one
    
    printf("\n Listening on localhost: port %d Users %d entries", port, MAX_USERS);
    printf("\n Listening on localhost: port %d", port);
    
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                            (struct sockaddr *)&remoteaddr,
                            &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                                "socket %d\n",
                                inet_ntop(remoteaddr.ss_family,
                                    get_in_addr((struct sockaddr*)&remoteaddr),
                                    remoteIP, INET6_ADDRSTRLEN),
                                newfd);
                    }
                } else {
                    if ((nbytes = recv(i, request, 100, 0)) <= 0) {
                        if (nbytes == 0) {
                            printf("selectserver:  hung up by user ");
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
			    printf("\n received %s", request);
			            /* Received a message parse it */
			            if (read_msg(request, tfile, tlast_modified, texpiry, host, tetag) ==1)
			            {
			                /* GET req for a valid host */
			                if (!tfile)
                            {
                                printf("\n Invalid filename: closing the conn");
                                close(i);
                                FD_CLR(i, &master);
                                continue;
                            }
			    cache_index = check_cache(tfile);
			                if (cache_index == -1)
                            {
                                /* Get the file & Create a new cache entry */
                                printf("\n Requested file not found in cache");
                                if(get_file_from_server(host, tfile, tlast_modified, texpiry, tetag)<0)
                                {
                                    perror("Error:");
                                    continue;
                                }
                                if((cache_index = insert_in_cache(tfile, tlast_modified, texpiry, tetag))==-1)
                                {
                                    perror("Error:");
                                    continue;
                                }
                            } else {
                                /* Entry already present: check expiry and give to client */
                                printf("\n Requested file found in cache\n");
                                if(check_expiry(cache_index, host, texpiry) ==2)
                                {
                                    printf("\n Page Not modified: Updating expiry time");
                                    strncpy(cache_array[cache_index]->expiry, texpiry, strlen(texpiry));
                                }
                                if(update_cache(cache_index))
                                {
                                    perror("\n Update cache failed");
                                    continue;
                                }
                            }
			    printf("\n The current cache:\n");
			    display_cache();
			    printf("\n\n");
                            send_response(i, cache_index);
                        }
                        memset(request, 0, 100);
                        close(i);
                        FD_CLR(i, &master);
                    }
                }
            }
        }
    }
    return 0;
}

