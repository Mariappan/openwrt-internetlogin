/***********************************************************
*           NetExpress Login Program
************************************************************/ 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_RECV_BUF_SIZE   (1024*1)
#define MAX_SEND_BUF_SIZE   (1024*2)
#define MAX_EXTRA_HEADER    (3)

#define MWGET_DEBUG 1

#if MWGET_DEBUG
#define mwdebug printf
#else
#define mwdebug(...)
#endif

enum {
    MWGET_GET_METHOD,
    MWGET_POST_METHOD,
} httpmethod;

static struct in_addr http_address;
static char *httpdata;
static char *http_url = "/"; // default url
static short int httpport = 80;
static char *http_header[MAX_EXTRA_HEADER];
static int hcount;

static void usage(const char *prog)
{
    printf("Usage: %s\n", prog);
    printf("%s <httpserver ip> p <port> m <GET/POST> u <uri> h <extra httpheader> d <data>\n", prog);
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
        }
        else if (!ret) {
            break;
        }

        len -= ret;
        data += ret;
    }

    return;
}

int mwget_parseargs(int argc, char **argv)
{
    int err = 0, i;
    char ch, *argvalue;

    if (argc%2 != 0) {
        return -1;
    }

    if(inet_aton(argv[1], &http_address) < 0) {
        return -1;
    }

    for (i = 2; i < argc; i += 2)
    {
        ch = *argv[i];
        argvalue = argv[i+1];

        switch(ch) {
        case 'p':
            httpport = atoi(argvalue);
            break;
        case 'm':
            if (strncmp(argvalue, "GET", 3) == 0) {
                httpmethod = MWGET_GET_METHOD;
            }
            else if (strncmp(argvalue, "POST", 4) == 0) {
                httpmethod = MWGET_POST_METHOD;
            }
            else {
                printf("Unknown method\n");
                err = -1;
                goto error;
            }
            break;
        case 'u':
            http_url = argvalue;
            break;
        case 'h':
            http_header[hcount++] = argvalue;
            break;
        case 'd':
            httpdata = argvalue;
            break;
        default:
            printf("Unknown Arg:%c\n", ch);
            err = -1;
            goto error;
        }
    }

error:
    return err;
}


int main (int argc, char **argv)
{
    int sock = -1;
    int err, index;
    char ch, *buf = NULL, *tmpbuf = NULL;
    char rcv_buf[MAX_RECV_BUF_SIZE];
    struct sockaddr_in serv_addr;

    if (mwget_parseargs(argc, argv) != 0) {
        usage(argv[0]);
        return -1;
    }

    mwdebug("addr:%s port:%u method:%s\n uri:%s extraheader:%s data:%s\n",
                inet_ntoa(http_address), httpport, (httpmethod==1)?"GET":"POST",
                http_url, http_header[0]?http_header[0]:"", httpdata?httpdata:"");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Create socket errror\n");
        return -1;
    }

    /* "address already in use" */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &err, sizeof(err))) {
        perror("setsockopt()");
        goto error;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(httpport);
    serv_addr.sin_addr.s_addr = http_address.s_addr;

    /* Now connect to the server */
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
         perror("ERROR connecting");
         exit(1);
    }

    mwdebug("Socket connected\n");

    buf = calloc(MAX_SEND_BUF_SIZE, 1);
    if (buf == NULL)
    {
        printf("No memory\n");
        goto error;
    }

    tmpbuf = buf;

    if (httpmethod == MWGET_GET_METHOD)
    {
        if (httpdata == NULL)
            sprintf(tmpbuf, "GET %s HTTP/1.0\r\n", http_url);
        else
            sprintf(tmpbuf, "GET %s?%s HTTP/1.0\r\n", http_url, httpdata);
    }
    else
    {
        sprintf(tmpbuf, "POST %s HTTP/1.0\r\n", http_url);
    }
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Host: %s\r\n", inet_ntoa(http_address));
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "User-Agent: Mozilla/5.0 (Ubuntu; Linux x86)\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "Accept: text/html, */*\r\n");
    tmpbuf += strlen(tmpbuf);

    /* Add custom header */
    for (index = 0; index < hcount ; index++)
    {
        sprintf(tmpbuf, "%s\r\n", http_header[index]);
        tmpbuf += strlen(tmpbuf);
    }

    if (httpdata && httpmethod == MWGET_POST_METHOD) {
        sprintf(tmpbuf, "Content-Type: application/x-www-form-urlencoded\r\n");
        tmpbuf += strlen(tmpbuf);

        sprintf(tmpbuf, "Content-length: %u\r\n", httpdata?(unsigned int)strlen(httpdata):0);
        tmpbuf += strlen(tmpbuf);
    }

    sprintf(tmpbuf, "Connection: close\r\n");
    tmpbuf += strlen(tmpbuf);

    sprintf(tmpbuf, "\r\n");
    tmpbuf += strlen(tmpbuf);

    /* HTTP Body for POST command */
    if(httpmethod == MWGET_POST_METHOD)
    {
        sprintf(tmpbuf, "%s", httpdata);
        tmpbuf += strlen(tmpbuf);
    }

    /* send http req */
    mwget_send(sock, buf, strlen(buf));

    while (1)
    {
        err = read(sock, &rcv_buf, MAX_RECV_BUF_SIZE);
        if (-1 > err) {
            perror("Read error");
            goto error;
        }
        else if (err == 0) {
            /* socket closed */
            goto error;
        }
        /* write to standard output */
        write (1, rcv_buf, err);
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
