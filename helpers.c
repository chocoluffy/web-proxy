//
// Created by xiangru on 10/24/18.
//

#include <string.h>
#include <stdio.h>

void parse_url(char *url, char *addr, char* port){
    int flag = 0;
    if (strlen(url) > 4){
        if (!(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p')){
            flag ++;
        }
    }
    int start = -1;
    int end = -1;
    int i;
    for(i = 0; i < strlen(url); i++){
        if (url[i] == ':')
            flag ++;
        if (flag == 2){
            if (start == -1) {
                start = i + 1;
                continue;
            }
            if (end == -1 && (url[i] < '0' || url[i] > '9')){
                end = i;
                break;
            }
        }
    }
    if (start == -1) {
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
        strcpy(addr, url);
        return;
    }
    if (end == -1)
        end = (int) (strlen(url));
    for (i = 0; i < start - 1; i ++)
        addr[i] = url[i];
    addr[start] = '\0';
    for (i = start; i < end; i ++)
        port[i - start] = url[i];
    port[end - start] = '\0';
}
