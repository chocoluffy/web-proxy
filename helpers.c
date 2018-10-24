//
// Created by xiangru on 10/24/18.
//

#include <string.h>
#include <stdio.h>

void parse_url(char *url, char *addr, char* port){
    int flag = 0, addr_start = -1, i;
    int port_start = -1, port_end = -1;
    if (strlen(url) > 4){
        if (!(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p')){
            flag ++;
            addr_start = 0;
        }
    }
    for(i = 0; i < strlen(url); i++){
        if (url[i] == ':') {
            flag++;
            if (addr_start == -1 && flag == 1){
                addr_start = i + 3;
            }
        }
        if (flag == 2){
            if (port_start == -1) {
                port_start = i + 1;
                continue;
            }
            if (port_end == -1 && (url[i] < '0' || url[i] > '9')){
                port_end = i;
                break;
            }
        }
    }
    if (port_start == -1) {
        strcpy(port, "80");
        strcpy(addr, url);
        return;
    }
    if (port_end == -1)
        port_end = (int) (strlen(url));
    memcpy(addr, &url[addr_start], (size_t) (port_start - addr_start - 1));
    memcpy(port,  &url[port_start], (size_t) (port_end - port_start));
    addr[port_start - addr_start - 1] = '\0';
    port[port_end - port_start] = '\0';
}