/***********************************************************
*           NetExpress Login Program
************************************************************/ 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>    /* for getopt */
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_RECV_BUF_SIZE   (1024*5)
#define MAX_HEADER          3

enum {
    MWGET_GET_METHOD,
    MWGET_POST_METHOD,
} httpmethod;

static int http_address;
static char *httpdata;
static short int httpport = 80;
static char *http_header[MAX_HEADER];
static int hcount;

int netx_is_header_recvd(char *buf)
{
    return 0;
}

int netx_packet_recvd(char *buf)
{
    return 0;
}

static void mwget_send(int fd, const char *data, int len)
{
    int ret;

    while (len) {
        ret = write(fd, data, len);
        if (ret < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            assert(0);
            /* consume all data */
            ret = len;
        }
        else if (!ret){
            break;
        }

        len -= ret;
        data += ret;
    }

    if (len) {
        assert(0);
    }

    return;
}


int main (int argc, char **argv)
{
    int sock = -1, yes;
    int recv_len, size;
    char ch, *buf = NULL, *tmpbuf = NULL;
    char *unknown_arg = "Unknown Arg";
    struct sockaddr_in serv_addr;

    while ((ch = getopt(argc, argv, "h:p:l:")) != -1) {
        switch(ch) {
        case 'l':
            index = optind-1;
            while(index < argc){
                next = strdup(argv[index]); /* get login */
                index++;
                if(next[0] != '-'){         /* check if optarg is next switch */
                    http_header[hcount++] = next;
                }
                else break;
            }
            break;
        case 'h':
            http_address = inet_aton(optarg);
            break;
        case 'p':
            httpport = atoi(optarg);
            break;
        case 'd':
            httpdata = optarg;
        default:
            write(2, unknown_arg, strlen(unknown_arg));
            return -1;
        }
    }

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
    serv_addr.sin_addr.s_addr = htonl(http_address);

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

    tmpbuf = buf;

    if (httpmethod == MWGET_GET_METHOD)
    {
        if (httpdata == NULL)
            sprintf(tmpbuf, "GET %s HTTP/1.1\r\n", inet_aton(http_address));
        else
            sprintf(tmpbuf, "GET %s?%s HTTP/1.1\r\n", inet_aton(http_address), httpdata);
    }
    else
    {
        sprintf(tmpbuf, "POST %s HTTP/1.1\r\n", inet_aton(http_address));
    }
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Accept: text/html, */*\r\n");
    tmpbuf += strlen(tmpbuf);

    /* Add custom header */
    for (i = 0; i < hcount ; i++)
    {
        sprintf(tmpbuf, "%s\r\n", http_header[i]);
        tmpbuf += strlen(tmpbuf);
    }

    sprintf(tmpbuf, "User-Agent: Mozilla/5.0\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Content-Type: application/x-www-form-urlencoded\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Content-length: %u\r\n", strlen(httpdata));
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Connection: close\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Cache-control: no-cache\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "\r\n");
    tmpbuf += strlen(tmpbuf);

    /* HTTP Body for POST command */
    if(httpmethod == MWGET_POST_METHOD)
    {
        sprintf(tmpbuf, "%s", httpdata);
        tmpbuf += strlen(tmpbuf);
    }

    mwget_send(sock, buf, strlen(buf));

    while (!netx_is_header_recvd(buf) && !netx_packet_recvd(buf))
    {
        size = read(sock, &buf[recv_len], MAX_RECV_BUF_SIZE - recv_len);
        if (-1 == size) {
            perror("Read error");
            goto error;
        }

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
