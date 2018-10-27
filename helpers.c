typedef struct LFU_entry{
    char* url;
    char* body;
    int freq;
} lfu_entry;

typedef struct Record{
    char* url;
    int freq;
} record;


void get_host_ip_and_port(char *url, char *addr, char* port){
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

void get_filename(char *url, char *filename){
    char *s = url;
    s = strstr(url, "http");
    if (s != NULL) {
        if (s[4] == 's')
            s += 1;
        s += 7;
    } else
        s = url;
    s = strchr(s, '/');
    strcpy(filename, s);
}


char* update_LFU(char *url, char *body, record *rec_table, lfu_entry *lfu, int *rec_tb_len){
    /**
     * Update LFU:
     * 1. If the given URL appear in the cache, then just update the frequency adding 1
     * 2. Otherwise, if the URL appeared before, increase its frequency by 1 and then check
     *    whether it is frequent enough to be in the cache. If so, update cache.
     * 3. If this is the first time this URL appears, add the URL to the record table with
     *    frequency 1.
     * In any of the cases, return the body (web content associated with the URL)
     */
    int min_entry = 0;
    for (int i =0; i< 3; i++){
        if (lfu[i].freq == 0){
            lfu[i].url = url;
            lfu[i].freq = 1;
            lfu[i].body = body;
            return body;
        }
        else {
            if (strcmp(lfu[i].url, url) == 0) {
                lfu[i].freq ++;
                return body;
            }
            if (lfu[i].freq < lfu[min_entry].freq){
                min_entry = i;
            }
        }
    }
    for (int i = 0; i < *rec_tb_len; i++){
        if (strcmp(rec_table[i].url, url) == 0) {
            rec_table[i].freq++;
            if (rec_table[i].freq >= lfu[min_entry].freq) {
                int tmp = rec_table[i].freq;
                rec_table[i].freq = lfu[min_entry].freq;
                rec_table[i].url = lfu[min_entry].url;
                lfu[min_entry].url = url;
                lfu[min_entry].freq = tmp;
                lfu[min_entry].body = body;
            }
            return body;
        }
    }
    rec_table[*rec_tb_len].url = url;
    rec_table[*rec_tb_len].freq = 1;
    *rec_tb_len = *rec_tb_len + 1;
    return body;
}
