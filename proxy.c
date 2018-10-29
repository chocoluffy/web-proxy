#include <stdio.h>
#include "csapp.h"
#include "helpers.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

pthread_mutex_t lock;

int parse_uri(char *uri, char *filename, char *cgiargs);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

struct thread_info {    /* Used as argument to update() */
    entry *lfu;
    entry *lru;
    record *rec_table;
    int rec_tb_len;
    int client_fd;
};


void *update(void *arg) {
    struct thread_info *tinfo = arg;
    printf("args client_fd: %d\n", tinfo->client_fd);
    int client_fd = tinfo->client_fd;
    entry *lfu = tinfo->lfu;
    entry *lru = tinfo->lru;
    record *rec_table = tinfo->rec_table;
    int rec_tb_len = tinfo->rec_tb_len;

    /**
     * 2. parse client request header. get server addr & port.
     */
    char buf[MAXLINE], method[MAXLINE], version[MAXLINE];
    char *uri;
    rio_t rio;
    printf("===\n");

    printf("client_fd: %d\n", client_fd);


    /* Read request line and headers */
    Rio_readinitb(&rio, client_fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    uri = malloc(MAXLINE * sizeof(char));
    sscanf(buf, "%s %s %s", method, uri,
           version);
    printf("method: %s, uri: %s, version: %s\n", method, uri, version);

    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(&rio, buf, MAXLINE);
    }

    /**
     * Hit Cache.
     */
    pthread_mutex_lock(&lock);
    char *lfu_cache_res = get_LFU(uri, lfu);
    char *lru_cache_res = get_LRU(uri, lru);
    pthread_mutex_unlock(&lock);
    if (lfu_cache_res != NULL) {
        pthread_mutex_lock(&lock);
        update_LFRU(uri, lfu_cache_res, rec_table, lfu, lru, &rec_tb_len, (int) time(NULL));
        pthread_mutex_unlock(&lock);

        Rio_writen(client_fd, lfu_cache_res, MAXBUF);
        printf("----------hit cache LFU-------------\n");
        printf("return response: %s\n", lfu_cache_res);
        printf("--------------------------------\n");
        Close(client_fd);
        return NULL;
    }
    if (lru_cache_res != NULL) {
        pthread_mutex_lock(&lock);
        update_LFRU(uri, lfu_cache_res, rec_table, lfu, lru, &rec_tb_len, (int) time(NULL));
        pthread_mutex_unlock(&lock);
        Rio_writen(client_fd, lru_cache_res, MAXBUF);

        printf("----------hit cache LRU-------------\n");
        printf("return response: %s\n", lru_cache_res);
        printf("--------------------------------\n");
        Close(client_fd);
        return NULL;
    }


    /**
     * 3. establish connection with server.
     * - assume we have ip: localhost; port: 3010.
     */
    struct sockaddr_in server;

    memset(&server, 0, sizeof(struct sockaddr_in));
    char hostname[MAXLINE], port[MAXLINE];
    get_host_ip_and_port(uri, hostname, port);
    printf("[3] parse url: hostname: %s, port: %s.\n", hostname, port);

    int server_fd = Open_clientfd(hostname, port);

    /**
     * 5. forward client's request to server.
     * - compare to serve_static().
     */
    char new_buff[MAXBUF];
    char filename[MAXLINE];
    get_filename(uri, filename);
    sprintf(new_buff,
            "GET %s HTTP/1.0\r\nHost: %s\r\n%sConnection: "
            "close\r\nProxy-Connection: close\r\n\r\n",
            filename, hostname, user_agent_hdr);
    // [TODO]: Finally, if a browser sends any additional request headers as
    // part of an HTTP request, your proxy should forward them unchanged.

    printf("[5] new buf content:\n %s", new_buff);

    Rio_writen(server_fd, new_buff, MAXLINE);

    rio_t s_rio;
    Rio_readinitb(&s_rio, server_fd);

    char *s_buf;
    s_buf = malloc(MAXBUF * sizeof(char));
    while (Rio_readnb(&s_rio, s_buf, MAXLINE) > 0) {
        printf("==s_buf:==\n");
        printf("%s", s_buf);
        Rio_writen(client_fd, s_buf, MAXBUF);
        // [TODO]: accumualte s_buf in case response body > 1 * MAXLINE. 
    }
    printf("===\n");

    // Save Server Response into cache.
    printf("[cache] current_read: %d, max: %d\n", (int) sizeof(s_buf), MAX_OBJECT_SIZE);
    if (sizeof(s_buf) < MAX_OBJECT_SIZE) {
        int now = (int) time(NULL);
        pthread_mutex_lock(&lock);
        update_LFRU(uri, s_buf, rec_table, lfu, lru, &rec_tb_len, now);
        pthread_mutex_unlock(&lock);

    }


    Close(client_fd);
    Close(server_fd);
    return NULL;
}


int main(int argc, char **argv) {
    printf("%s", user_agent_hdr);
    int proxy_fd;
    int client_fd;  // use for accept\send to client.
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t client_len;
    struct sockaddr_storage client_addr;

    entry lfu[3];
    entry lru[1000];
    // Initalize the lfu array.
    for (int i = 0; i < 3; i++) {
        lfu[i].url = NULL;
        lfu[i].freq = 0;
        lfu[i].body = NULL;
        lfu[i].time = 0;
    }

    for (int i = 0; i < 1000; i++) {
        lru[i].url = NULL;
        lru[i].freq = 0;
        lru[i].body = NULL;
        lru[i].time = 0;
    }

    record rec_table[1000];
    int rec_tb_len = 0;

    /*
     * 1. establish connection with client: bind argv[1] to proxy_fd.
     * 2. parse client request header. get server addr & port.
     * 3. eatablish connection with server.
     * 4. ... (TODO: check and update header.)
     * 5. forward client's request to server.
     * 6. wait for response from server.
     * 7. forward response to client.
     */

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    proxy_fd = Open_listenfd(argv[1]);
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }

    while (1) {

        printf("\n\n\n");
        printf("\n\n\n");

        //   printf("----------position 0-------------\n");

        // for(int i = 0; i < 3; i++) {
        //     printf("[lfu entry]: url: %s, body: %s, fre: %d.\n", lfu[i].url, lfu[i].body, lfu[i].freq);
        // }
        //   printf("--------------------------------\n");


        // 1. establish connection with client: bind argv[1] to proxy_fd.
        client_len = sizeof(client_addr);

        client_fd = Accept(proxy_fd, (SA *) &client_addr,
                           &client_len);  // line:netp:tiny:accept
        Getnameinfo((SA *) &client_addr, client_len, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("[start]: Accepted connection from (%s, %s)\n", hostname, port);

        // printf("----------LFU cache. after connection-------------\n");

        // for(int i = 0; i < 3; i++) {
        //     printf("[lfu entry]: url: %s, body: %s, fre: %d.\n", lfu[i].url, lfu[i].body, lfu[i].freq);
        // }
        // printf("--------------------------------\n");

        // Each client connection takes one thread.
        // int pid = Fork();
        // if (pid < 0) {
        //   Close(client_fd);
        //   break;
        // }
        // if (pid == 0) {


        // Use pthread, to create a new thread each time having a new connection with client.
        struct thread_info *tinfo = calloc(1, sizeof(struct thread_info));
        pthread_t *tid = malloc(1 * sizeof(pthread_t));

        tinfo->lfu = lfu;
        tinfo->lru = lru;
        tinfo->rec_table = rec_table;
        tinfo->rec_tb_len = rec_tb_len;
        tinfo->client_fd = client_fd;

        // printf("args client_fd: %d\n", tinfo->client_fd);

        pthread_create(tid, NULL, &update, tinfo);

        printf("\n\n*************************");

        printf("[LFU cache] update cache success!\n");
        printf("----------update cache with server response-------------\n");
        for (int i = 0; i < 3; i++) {
            printf("[lfu entry]: url: %s, body: %s, fre: %d.\n", lfu[i].url, lfu[i].body, lfu[i].freq);
        }
        printf("--------------------------------------------------------\n");

        printf("[LRU cache] update cache success!\n");
        printf("----------update cache with server response-------------\n");
        for (int i = 0; i < 3; i++) {
            printf("[lru entry]: url: %s, body: %s, fre: %d, time: %d.\n", lru[i].url, lru[i].body, lru[i].freq,
                   lru[i].time);
        }
        printf("--------------------------------------------------------\n");

        printf("\n\n");

        // break;
    }
    // }

    pthread_mutex_destroy(&lock);
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {
        /* Static content */              // line:netp:parseuri:isstatic
        strcpy(cgiargs, "");              // line:netp:parseuri:clearcgi
        strcpy(filename, ".");            // line:netp:parseuri:beginconvert1
        strcat(filename, uri);            // line:netp:parseuri:endconvert1
        if (uri[strlen(uri) - 1] == '/')  // line:netp:parseuri:slashcheck
            strcat(filename, "home.html");  // line:netp:parseuri:appenddefault
        return 1;
    } else { /* Dynamic content */  // line:netp:parseuri:isdynamic
        ptr = index(uri, '?');        // line:netp:parseuri:beginextract
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        } else
            strcpy(cgiargs, "");  // line:netp:parseuri:endextract
        strcpy(filename, ".");  // line:netp:parseuri:beginconvert2
        strcat(filename, uri);  // line:netp:parseuri:endconvert2
        return 0;
    }
}
/* $end parse_uri */