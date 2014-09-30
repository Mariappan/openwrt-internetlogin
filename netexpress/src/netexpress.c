/***********************************************************
*           NetExpress Login Program
************************************************************/ 

#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_RECV_BUF_SIZE   (1024*5)

int httpport = 80;

int netx_is_header_recvd(char *buf)
{
    return -1;
}

int netx_packet_recvd(char *buf)
{
    return -1;
}

int main(void)
{
    int sock = -1, yes;
    int recv_len, size;
    char *buf = NULL;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock)
    {
        printf("Create socket errror\n");
        return -1;
    }

    /* "address already in use" */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
        perror("setsockopt()");
        goto error;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(httpport);
    serv_addr.sin_addr.s_addr = inet_aton("1.254.254.254");

    /* Now connect to the server */
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
         perror("ERROR connecting");
         exit(1);
    }


    buf = calloc(MAX_RECV_BUF_SIZE, 1);
    if (buf == NULL)
    {
        printf("No memory\n");
        goto error;
    }

    while (!netx_is_header_recvd(buf) && !netx_packet_recvd(buf))
    {
        size = read(sock, buf[recv_len], MAX_RECV_BUF_SIZE - recv_len);

        if ((size + recv_len) > MAX_RECV_BUF_SIZE) {
            assert(0);
        }

        recv_len += size;
    }

    
error:

    if (buf) {
        free(buf);
    }

    if (-1 != sock) {
        close(sock);
    }

    return 0; 
}
