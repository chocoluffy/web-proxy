#include <stdio.h>
#include "csapp.h"
#include "helpers.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Initilize mutex lock. */
pthread_mutex_t lock;

/* Define user agent. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/**
 * Argument struct when passing into new thread execution.
 */
struct thread_info {
  entry *lfu;
  entry *lru;
  record *rec_table;
  int rec_tb_len;
  int client_fd;
};

/**
 * Each new client connection will spawn a new thread.
 */
void *update(void *arg) {
  struct thread_info *tinfo = arg;
  int client_fd = tinfo->client_fd;
  entry *lfu = tinfo->lfu;
  entry *lru = tinfo->lru;
  record *rec_table = tinfo->rec_table;
  int rec_tb_len = tinfo->rec_tb_len;

  /* Parse client request header. get server addr & port. */
  char buf[MAXLINE], method[MAXLINE], version[MAXLINE];
  char *uri;
  rio_t rio;

  /* Read request line and headers. */
  Rio_readinitb(&rio, client_fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  uri = malloc(MAXLINE * sizeof(char));
  sscanf(buf, "%s %s %s", method, uri, version);

  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(&rio, buf, MAXLINE);
  }

  /* Hit Cache. */
  pthread_mutex_lock(&lock);
  char *lfu_cache_res = get_LFU(uri, lfu);
  char *lru_cache_res = get_LRU(uri, lru);
  pthread_mutex_unlock(&lock);
  if (lfu_cache_res != NULL) {
    pthread_mutex_lock(&lock);
    update_LFRU(uri, lfu_cache_res, rec_table, lfu, lru, &rec_tb_len,
                (int)time(NULL));
    pthread_mutex_unlock(&lock);

    Rio_writen(client_fd, lfu_cache_res, MAXBUF);
    Close(client_fd);
    return NULL;
  }
  if (lru_cache_res != NULL) {
    pthread_mutex_lock(&lock);
    update_LFRU(uri, lfu_cache_res, rec_table, lfu, lru, &rec_tb_len,
                (int)time(NULL));
    pthread_mutex_unlock(&lock);
    Rio_writen(client_fd, lru_cache_res, MAXBUF);

    Close(client_fd);
    return NULL;
  }

  /* Establish connection with server. */
  struct sockaddr_in server;
  memset(&server, 0, sizeof(struct sockaddr_in));
  char hostname[MAXLINE], port[MAXLINE];
  get_host_ip_and_port(uri, hostname, port);

  int server_fd = Open_clientfd(hostname, port);

  /* Forward client's request to server. */
  char new_buff[MAXBUF];
  char filename[MAXLINE];
  get_filename(uri, filename);
  sprintf(new_buff,
          "GET %s HTTP/1.0\r\nHost: %s\r\n%sConnection: "
          "close\r\nProxy-Connection: close\r\n\r\n",
          filename, hostname, user_agent_hdr);

  Rio_writen(server_fd, new_buff, MAXLINE);

  /* Receive response from server. */
  rio_t s_rio;
  Rio_readinitb(&s_rio, server_fd);

  char *s_buf;
  s_buf = malloc(MAXBUF * sizeof(char));
  int total_size = 0;
  while (Rio_readnb(&s_rio, s_buf, MAXLINE) > 0) {
    Rio_writen(client_fd, s_buf, MAXBUF);
    total_size += strlen(s_buf);
  }

  /* Only response body < MAX_OBJECT_SIZE will be stored in cache. */
  if (total_size < MAX_OBJECT_SIZE) {
    int now = (int)time(NULL);
    pthread_mutex_lock(&lock);
    update_LFRU(uri, s_buf, rec_table, lfu, lru, &rec_tb_len, now);
    pthread_mutex_unlock(&lock);
  }

  Close(client_fd);
  Close(server_fd);
  return NULL;
}

int main(int argc, char **argv) {
  int proxy_fd;
  int client_fd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t client_len;
  struct sockaddr_storage client_addr;

  entry lfu[3];
  entry lru[1000];
  /* Initilize the LFU array. */
  for (int i = 0; i < 3; i++) {
    lfu[i].url = NULL;
    lfu[i].freq = 0;
    lfu[i].body = NULL;
    lfu[i].time = 0;
  }

  /* Initilize the LRU array. */
  for (int i = 0; i < 1000; i++) {
    lru[i].url = NULL;
    lru[i].freq = 0;
    lru[i].body = NULL;
    lru[i].time = 0;
  }

  record rec_table[1000];
  int rec_tb_len = 0;

  /* Check command line args. */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  proxy_fd = Open_listenfd(argv[1]);
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("\n mutex init failed\n");
    return 1;
  }

  /* Proxy always alive, listening for connection from clients. */
  while (1) {
    client_len = sizeof(client_addr);
    client_fd = Accept(proxy_fd, (SA *)&client_addr, &client_len);
    Getnameinfo((SA *)&client_addr, client_len, hostname, MAXLINE, port,
                MAXLINE, 0);

    /* Initilize a new thread for a new client connection. */
    struct thread_info *tinfo = calloc(1, sizeof(struct thread_info));
    pthread_t *tid = malloc(1 * sizeof(pthread_t));

    tinfo->lfu = lfu;
    tinfo->lru = lru;
    tinfo->rec_table = rec_table;
    tinfo->rec_tb_len = rec_tb_len;
    tinfo->client_fd = client_fd;

    /* New thread execution. */
    pthread_create(tid, NULL, &update, tinfo);
  }

  pthread_mutex_destroy(&lock);
}