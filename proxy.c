#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);
  int proxy_fd, client_fd, server_fd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t client_len, server_len;
  struct sockaddr_storage client_addr, server_addr;

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

  while (1) {
    client_len = sizeof(client_addr);
    client_fd = Accept(proxy_fd, (SA *)&client_addr,
                       &client_len);  // line:netp:tiny:accept
    Getnameinfo((SA *)&client_addr, client_len, hostname, MAXLINE, port,
                MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    
    /**
     * 2.
     */
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, client_fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { //line:netp:doit:readrequest
        return -1; /* TODO: cannot break the proxy. need error handling. */
    }
    printf("%s\n", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    printf("[2] method: %s, uri: %s, version: %s\n", method, uri, version);
    read_requesthdrs(&rio); 
    



    // doit(client_fd);   // line:netp:tiny:doit
    Close(client_fd);  // line:netp:tiny:close
  }
  
  return 0;
}