#include <stdio.h>
#include "csapp.h"
#include "helpers.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


int parse_uri(char *uri, char *filename, char *cgiargs);
void get_host_ip_and_port(char* uri, char* name, char* port);


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);
  int proxy_fd;
  int client_fd; // use for accept\send to client.
  int server_fd; // use for connect\send to the server.
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
    // 1. establish connection with client: bind argv[1] to proxy_fd.
    client_len = sizeof(client_addr); 

    client_fd = Accept(proxy_fd, (SA *)&client_addr,
                       &client_len);  // line:netp:tiny:accept
    Getnameinfo((SA *)&client_addr, client_len, hostname, MAXLINE, port,
                MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    
    /**
     * 2. parse client request header. get server addr & port.
     */
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, client_fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { //line:netp:doit:readrequest
        return -1; /* TODO: cannot break the proxy. need error handling. */
    }
    // printf("%s\n", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    printf("[2] method: %s, uri: %s, version: %s\n", method, uri, version);
    // read_requesthdrs(&rio); 
    int is_static;
    char filename[MAXLINE], cgiargs[MAXLINE];
    is_static = parse_uri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
    printf("[2] uri: %s, filename: %s, cgi: %s\n", uri, filename, cgiargs);

    /**
     * 3. eatablish connection with server.
     * - assume we have ip: localhost; port: 3010.
     */
    struct sockaddr_in server;
    struct hostent *he;	 /* structure for resolving names into IP addresses */
    socklen_t socklen;	 /* length of the socket structure sockaddr         */

	  memset(&server, 0, sizeof(struct sockaddr_in));
    char hostname[MAXLINE], port[MAXLINE];
    get_host_ip_and_port(uri, hostname, port);
    printf("[3] parse url: hostname: %s, port: %s.\n", hostname, port);

    int server_fd = Open_clientfd(hostname, port);

    /**
     * 4. ... (TODO: check and update header.)
     */

    /**
     * 5. forward client's request to server.
     * - compare to serve_static().
     */
    char* new_buff = "GET /home.html HTTP/1.1\r\n\r\n";
    printf("[5] new buf content: %s\n", new_buff);

    int send_res = Write(server_fd, new_buff, sizeof(new_buff));
    if (send_res < 0) {
      perror("[5] Write.");
    }
    printf("[5] send response: %d.\n", send_res);


    /**
     * 6. wait for response from server.
     */
    rio_t s_rio;
    char s_buf[MAXLINE];

    /* Read request line and headers */
    Rio_readinitb(&s_rio, server_fd);
    if (!Rio_readlineb(&s_rio, s_buf, MAXLINE)) { //line:netp:doit:readrequest
        return -1; /* TODO: cannot break the proxy. need error handling. */
    }
    printf("[6] s_buf content: %s\n", s_buf);

    /**
     * 7. forward response to client
     */
    // int send_res2 = Write(client_fd, s_buf, sizeof(s_buf));
    // Write(client_fd, "\r\n", sizeof("\r\n"));

    // if (send_res2 < 0) {
    //   perror("[7] Write.");
    // }
    // printf("[7] send response: %d.\n", send_res2);

    // doit(client_fd);   // line:netp:tiny:doit
    Close(client_fd);  // line:netp:tiny:close
  }
  
  return 0;
}


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
	return 1;
    }
    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
	strcat(filename, uri);                           //line:netp:parseuri:endconvert2
	return 0;
    }
}
/* $end parse_uri */