/*
 * To parse and form the following
 * Connects to server
 * Sends messages to server
 * receives messages from server
 */ 
/*
 * Message format
 * VER(9)
 */ 



#include <common.h>
#define PORT 9034

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


/*
 * SBCP JOIN MSG
 * @in: 
 * @out:
 * @return: -1 for FAIL
 *           0 for SUCCESS
 */ 
int  join_request(char *username, int *r_len, my_packet *packet)
{
    bool type[7] = {'0', '1', '0', '0', '0', '0', '0'};
    uint16_t vertype =0, len, i=0;

    
    vertype = join_ver_type(type);

    len = sizeof(uint16_t) + sizeof(uint16_t) + strlen(username)+1; 
    if (len > 8) 
    {
       //ERROR
    }
    *r_len = len + sizeof(vertype) + sizeof(len);
   
    /*
     * copy all the above data into packet 
     */ 
    packet->vertype = vertype;
    packet->length = len;
    packet->attrtype = USERNAME; 
    packet->attrlen = strlen(username) ;
    len = strlen(username);
    memcpy(packet->buf, username, len);
    //printf("\nThe username given %s \t%d\n", username, len);
    for (i = len; i< 50; i++)
    {
	    packet->buf[i] = '\0';
	
    }
    packet->buf[len] = '\0';	
    len = strlen(packet->buf);
    //printf("The username copied %s \t %d\n", packet->buf, len);
    

    //printf("\n werid: I dont get printed IDKY :D");


    return 0;
}

int send_msg(char *text, my_packet *packet)
{

    bool type[7] = {'0', '0', '1', '0', '0', '0', '0'};
    uint16_t vertype, len;
    
    
    vertype = join_ver_type(type);
    len = strlen(text);

    if (len ==1 && (*text=='\n'))
            return 0;
    len = strlen(text);
    //printf("\n text len %d %s", len, text);

    len = sizeof(uint16_t) + sizeof(uint16_t) + strlen(text); 
    /*
     * Form the send packet
     */
    packet->vertype = vertype;
    packet->length = len + sizeof vertype + sizeof len;
    packet->attrtype = MSG;
    packet->attrlen = strlen(text);

    strncpy(packet->buf, text, 50);
    packet->buf[packet->attrlen] = '\0';

    return 0;
}

int read_msg (my_packet packet, char *buf)
{
    uint16_t mask_version = 128;
    uint8_t mask_type=15, type, *t;
    uint16_t vertype;

    /*
     * Check the version and type
     */
    vertype = packet.vertype;
    t = (uint8_t *) &vertype;
    type = *t;
    t++;
    //printf("\n vertype = %d", packet.vertype);
    mask_version = mask_version&type;
    if (mask_version == 128)
    {
        if(*t ==1)
        {
            //printf("Version match");
        } else {
            printf("1No Version match");
        }
    } else {
            printf("2NO Version match");
    }

    type&=mask_type;

    switch(type)
    {
        case FWD:
            //printf("\n server FWDed Packet ");
            strncpy(buf, packet.buf, 50);
            type = strlen(packet.buf);
            buf[packet.attrlen+1] = '\0';
            break;
        default:
            printf("\n Packet drop");
            buf = NULL;
            break;
    }
    return 0;
}


int main(int argc, char** argv) {
    int csd;
    uint16_t port;
    struct sockaddr_in addr;
    struct hostent* hret;
    char msg[50];
    int n_bytes = 0, i;
    char *username;
    my_packet packet;

    fd_set master;
    fd_set read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    username = (char *)malloc(20);

    memset(username, 0, 20);
    port = atoi(argv[2]);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    username = argv[3];
   
    hret = gethostbyname(argv[1]);
    memcpy(&addr.sin_addr.s_addr, hret->h_addr,
            hret->h_length);

    if((csd = socket(AF_INET, SOCK_STREAM, 0))
             == -1) {
        perror("Cannot create"
               "client socket");
        exit(1);
    }
    fprintf(stderr,"socket created \n");

    if(connect(csd, (struct sockaddr*)&addr,
                sizeof(struct sockaddr)) == -1) {
        perror("Cannot connect to"
                "Server");
        exit(1);
    }

    FD_SET(csd, &master);
    FD_SET(0, &master);
    fdmax = csd;

    //printf("\nThe username entered %s", username);
    /* Send JOIN request */
    join_request(username, &n_bytes, &packet); 
    //printf("\njoin req packet bytes %d", n_bytes);
    //write(csd, (void*)&packet, n_bytes);
    if (send(csd, &packet, n_bytes, 0) == -1) {
        perror("\n send");
    }
    memset(&packet, 0, sizeof packet +50);
    //printf("\njoin req sent ");
    
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { 
                if (i == csd) {
                    /* Got something from server */
	                if ((n_bytes = recv(csd, (my_packet *)&packet, 50+sizeof (packet), 0)) <= 0) {
                         /* Conn closed by server */
                         perror("Error: User name not available: Conn closed by server");
                         close(csd);
                         FD_CLR(csd, &master);
                         FD_CLR(0, &master);
                         exit(0);
                    }
                    //printf("\n rxd msg %s l %d aty %d atl%d\n", packet.buf,
                       // packet.length, packet.attrtype, packet.attrlen);
                    read_msg(packet, (char *)msg);
                    if (msg[0]=='\0' || msg[0]=='\n')
                    {   
                        printf("\n No msg");
                        continue;
                    }
                    n_bytes = strlen(msg)+1;
                    write(1, msg, n_bytes);
                    memset(&packet, 0, sizeof(packet)+50);
                    //send(1, msg, n_bytes, 0);
                } else {
                    /* input from keyboard */
                    n_bytes = read(0, msg, 50);
                    //n_bytes = recv(0, msg, 50, 0);
                    if (msg[0] == '\0' || msg[0] == '\n')
                    {
                        memset(msg, 0, 50);
                    } else {
                    send_msg(msg, &packet);
                    //printf("\n sending a msg %s l %d aty %d atl%d\n", packet.buf,
                           // packet.length, packet.attrtype, packet.attrlen);
                    if (send(csd, (void *)&packet, packet.length, 0) == -1) {
                        perror("send");
                    }
                    memset(&packet, 0, sizeof(packet)+50);
                    }

                }
            }
        }
    }

    //write(1, msg, n_bytes);
    return 0;
}

