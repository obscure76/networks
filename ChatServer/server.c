/*
 * Server.c is responsible for server functionality
 * server adds a user and 
 */ 

#include<common.h>

char *user_array[30];
int user_count = 0;
int index_socket[20];


int check_username(char *username)
{
    uint8_t len, i;
    len = sizeof username;
    for (i =0; i < 10; i++)
    {
        if (user_array[i])
        {
            //printf("\n c%s   s%s", username, user_array[i]);
            return strncmp(username, user_array[i], len);
        }
    }

    return 1; 
}


uint16_t join_ver_type(bool type[7])
{
    int i;
    bool vertyp[16];
    uint16_t combine = 0;
    uint16_t pow=1;
    bool ver[9] = {'1', '1', '0', '0', '0', '0', '0', '0', '0'};

    for (i=0;i<7;i++)
    {
        vertyp[i] = type[i];
    }
        
    for (i=15;i>6;i--)
    {
        vertyp[i] = ver[i-7];
    }

    for (i=0;i<16;i++)
    {
        if (vertyp[i] == '1') {
            combine+= pow;
        }
        pow*=2;
    }
    return combine;
}


int fwd_msg(my_packet *packet,char *text)
{
    bool type[7] = {'1', '1', '0', '0', '0', '0', '0'};
    uint16_t vertyp, len;

    vertyp = join_ver_type(type);
    len = strlen(text);
    if(len ==1 && (*text== '\0' || *text== '\n'))
    {
        packet = NULL;
        return 0;
    }

    len = sizeof(uint16_t) + sizeof(uint16_t) + len;

    /*
     * * Form the send packet
     * */
    packet->vertype = vertyp;
    packet->attrtype = FWD;
    packet->attrlen = strlen(text);

    len = strlen(text);
    strncpy(packet->buf, text, 50);
    packet->buf[len+1] = '\0';

    //printf("\n this str %s will be fwded\n", text);
    //printf("\n this str %s will be fwded\n", packet->buf);
    return 0;
}

int read_msg(my_packet *packet, int sock_id, my_packet *f_packet, int *nbytes, int MAX_USERS)
{
    uint8_t mask_version = 128, index;
    uint8_t mask_type=15, type, *t, i, ret_val =1;
    uint16_t vertype;
    uint16_t len;
    char buf[100];

    vertype = packet->vertype;
    t = (uint8_t *) &vertype;
    type = *t;
    t++;
    mask_version = mask_version&type;
    if (mask_version == 128)
    {
        if(*t ==1)
        {
            //printf("Version match");
        } else {
            printf("Version no match");
            ret_val = 2;
            return ret_val;
        }

    } else {
        printf(" Version no match");
        ret_val = 2;
        return ret_val;
    }

    type&=mask_type;
    
    switch(type) {
        case 2:
            /* JOIN REQ */
            printf("\n JOIN REQ from %s\n", packet->buf);
            if (!check_username((char *)packet->buf))
            {
                printf("\n The userame %s not available", packet->buf);
                ret_val =2;
                break;
            }
            for (i=0;i<10;i++) 
            {
                if (!user_array[i])
                    break;
            }
            if (i == 10 || user_count >= MAX_USERS) 
            {
                printf("\n The users are full:");
                ret_val =2;
                break;
            }
            len = strlen(packet->buf);
            user_array[i] = (char *)malloc(len+1);
            strncpy(user_array[i], packet->buf, len);
            index_socket[sock_id] = i;
            user_count++;
            f_packet = NULL;
            memset(packet, 0, sizeof packet + 50);
            break;
            
        case 4:
            /* SENT MSG: Append Username & FWD it */
            ret_val =1;
            //printf("\n  Rxd this msg %s l %d aty %datl %d\n", packet->buf,
              //      packet->length, packet->attrtype, packet->attrlen);
            memset(buf, 0, 100);
            index = index_socket[sock_id];
            len =  strlen(user_array[index]);
            strncpy(buf, user_array[index], len);
            buf[len] = ':';
            buf[len+1] = '\0';
            //printf("\n The user_array name%d  %s", len, buf);
            len = strlen(packet->buf);
            //printf("\n The text %d  %s", len, packet->buf);
            strncat(buf, packet->buf, len);
            fwd_msg(f_packet, buf);
            len = 8 + strlen(f_packet->buf);
            //printf("\n The ftext %d  %s", len, f_packet->buf);
            *nbytes = 8 + len;
            ret_val =0;
            break;

        default:
            /* Unknown type do nothing*/
            printf("\n Packet drop");
            break;
    }

    //printf("\n IDK if i get printed :D");

    return ret_val; 
}

#define PORT "9034"


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
    my_packet *packet;
    int nbytes;
    int yes =1;
    char remoteIP[INET6_ADDRSTRLEN];
    int i, j, rv, ch, index;
    struct addrinfo hints, *ai, *p;
    struct sockaddr_in addr;
    my_packet *f_packet;
    uint16_t port;
    char g_port[6];
    uint8_t MAX_USERS;

    packet = (my_packet *) malloc(sizeof(my_packet));
    f_packet = (my_packet *) malloc(sizeof(my_packet));
    memset(packet, 0, sizeof(my_packet));
    memset(f_packet, 0, sizeof(my_packet));


    
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    MAX_USERS = atoi(argv[3]);
    
    for (i=0,j=0;j<30&&i<20;j++,i+=2)
    {
        user_array[j]=NULL;
        user_array[j+1]= NULL;
        user_array[j+2] =NULL;
        index_socket[i] = -1;
        index_socket[i+1] = -1;
    }
    port = atoi(argv[2]); 
    memcpy(g_port, argv[2], sizeof(argv[2])); 
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    //write(stdout, "The bind is done\0",16);
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    /*
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot bind socket");
        exit(1);
    }
    if(bind(listener, (struct sockaddr*)&addr,
                sizeof(struct sockaddr)) == -1) {
        perror("CCannot bind socket");
        exit(1);
    }
    */

    freeaddrinfo(ai); // all done with this
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    FD_SET(listener, &master);
    fdmax = listener; // so far, it's this one
    printf("\n Listening on localhost: port %d", 9034);
    printf("\n Listening on localhost: port %d", 9034);
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
                    if ((nbytes = recv(i, (my_packet *)packet, 50+sizeof (packet), 0)) <= 0) {
                        if (nbytes == 0) {
                            printf("selectserver:  hung up by user %s\n", user_array[index_socket[i]]);
                        } else {
                            perror("recv");
                        }
                        index = index_socket[i];
                        free(user_array[index]);
                        user_array[index]=NULL;
                        user_count--;
                        index_socket[i] = -1;
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
			            /* Received a message from a client: parse it */
			            if ((ch= read_msg(packet, i, f_packet, &nbytes, MAX_USERS))){
			                //printf("\n packet is join req:");
                            if (ch == 2) 
                            {
                            /* Some ERROR : close the conn*/
			                printf("\n username in use: close the conn");
                            if (ch == 2) 
                                close(i); // bye!
                                FD_CLR(i, &master); // remove from master set
                            }
			                continue;
                        }
                        if (!f_packet) 
                            continue;
                        //printf("\n fwding a msg %s l %d aty %d atl%d\n", f_packet->buf,
                          //      f_packet->length, f_packet->attrtype, f_packet->attrlen);
                        //printf("\n the num of bytes being sent %d", nbytes);
                        for(j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master)) {
                                if (j != listener && j != i) {
                                    if (send(j, f_packet, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                        memset(packet, 0, sizeof(packet) +50);
                    }
                }
            }
        }
    }
    return 0;
}

