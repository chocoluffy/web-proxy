typedef struct entry{
    char* url;
    char* body;
    int freq;
    int time;
} entry;


typedef struct Record{
    char* url;
    int freq;
    int time;
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

void update_LFU(char *url, char *body, record *rec_table, entry *lfu, int *rec_tb_len, int time){
    /**
     * Update LFU:
     * 1. If the given URL appear in the cache, then just update the frequency adding 1
     * 2. Otherwise, if the URL appeared in records, increase its frequency by 1 and then check
     *    whether it is frequent enough to be in the cache. If so, update cache.
     * 3. If this is the first time this URL appears, add the URL to the record table with
     *    frequency 1.
     */
    int min_entry = 0;
    for (int i =0; i< 3; i++){
        if (lfu[i].freq == 0){
            lfu[i].url = url;
            lfu[i].freq = 1;
            lfu[i].time = time;
            lfu[i].body = body;
            return;
        }
        else {
            if (strcmp(lfu[i].url, url) == 0) {
                lfu[i].freq ++;
                return;
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
                int tmp1 = rec_table[i].freq;
                int tmp2 = rec_table[i].time;
                rec_table[i].url = lfu[min_entry].url;
                rec_table[i].freq = lfu[min_entry].freq;
                rec_table[i].time = lfu[min_entry].time;
                lfu[min_entry].url = url;
                lfu[min_entry].freq = tmp1;
                lfu[min_entry].time = tmp2;
                lfu[min_entry].body = body;
            }
            return;
        }
    }
    rec_table[*rec_tb_len].url = url;
    rec_table[*rec_tb_len].freq = 1;
    rec_table[*rec_tb_len].time = time;
    *rec_tb_len = *rec_tb_len + 1;
}

void update_LRU(char *url, char *body, record *rec_table, entry *lru, int *rec_tb_len, int time) {
    /**
     * Update LRU:
     * 1. If the given URL appear in the cache, then just update the time to the given time
     * 2. Otherwise, if the URL appeared in records, update the time to the given time and then
     *    exchange this record with the cache which is the oldest
     * 3. If this is the first time this URL appears, first append it to the end of the records,
     *    then exchange it with the oldest cache
     */
    int min_entry = 0;
    for (int i = 0; i < 3; i++) {
        if (lru[i].time == 0) {
            lru[i].url = url;
            lru[i].time = time;
            lru[i].freq = 1;
            lru[i].body = body;
            return;
        } else {
            if (strcmp(lru[i].url, url) == 0) {
                lru[i].time = time;
                // lru[i].freq ++;
                return;
            }
            if (lru[i].time < lru[min_entry].time) {
                min_entry = i;
            }
        }
    }
    for (int i = 0; i < *rec_tb_len; i++) {
        if (strcmp(rec_table[i].url, url) == 0) {
            rec_table[i].time = time;
            // rec_table[i].freq ++;
            if (rec_table[i].time >= lru[min_entry].time) {
                int tmp1 = rec_table[i].freq;
                int tmp2 = rec_table[i].time;
                rec_table[i].url =  lru[min_entry].url;
                rec_table[i].freq = lru[min_entry].freq;
                rec_table[i].time = lru[min_entry].time;
                lru[min_entry].url = url;
                lru[min_entry].freq = tmp1;
                lru[min_entry].time = tmp2;
                lru[min_entry].body = body;
            }
            return;
        }
    }
    rec_table[*rec_tb_len].url = url;
    rec_table[*rec_tb_len].freq = 1;
    rec_table[*rec_tb_len].time = time;
    if (rec_table[*rec_tb_len].time >= lru[min_entry].time) {
        int tmp1 = rec_table[*rec_tb_len].freq;
        int tmp2 = rec_table[*rec_tb_len].time;
        rec_table[*rec_tb_len].url =  lru[min_entry].url;
        rec_table[*rec_tb_len].freq = lru[min_entry].freq;
        rec_table[*rec_tb_len].time = lru[min_entry].time;
        lru[min_entry].url = url;
        lru[min_entry].freq = tmp1;
        lru[min_entry].time = tmp2;
        lru[min_entry].body = body;
    }
    *rec_tb_len = *rec_tb_len + 1;
}

char* get_LFU(char *url, entry *lfu) {
    /**
     * return NULL: content from this url is not cached in lfu table.
     */
    for (int i = 0; i < 3; i++) {
        if (lfu[i].freq == 0) return NULL; // lfu_table has not been initialized.
        if (strcmp(url, lfu[i].url) == 0) {
            printf("[get_LFU]: cache key: %s, my key: %s\n", lfu[i].url, url);
            printf("[get_LFU]: cache key: %s, body: %s\n", lfu[i].url, lfu[i].body);
            return lfu[i].body;
        }
    }
    return NULL;
}

char* get_LRU(char *url, entry *lru) {
    /**
     * return NULL: content from this url is not cached in lfu table.
     */
    for (int i = 0; i < 1000; i++) {
        if (lru[i].freq == 0) return NULL; // lfu_table has not been initialized.
        if (strcmp(url, lru[i].url) == 0) {
            printf("[get_LRU]: cache key: %s, my key: %s\n", lru[i].url, url);
            return lru[i].body;
            // printf("[get_LFU]: res: %s\n", res);
        }
    }
    return NULL;
}

//int main(){
//    lfu_entry lfu[3];
//    record rec_table[1000];
//    int rec_tb_len = 0;
//    update_LFU("a.com", "aaaaa", rec_table, lfu, &rec_tb_len);
//    return 0;
//}