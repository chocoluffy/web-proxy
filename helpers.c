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


/**
 * Parse client url to obtain ip address and port information.
 */
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


/**
 * Parse url to get filename.
 */
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

/**
 * Update LFU and LRU. 
 */
void update_LFRU(char *url, char *body, record *rec_table, entry *lfu, entry *lru, int *rec_tb_len, int time) {
    int i, j, min_freq = 0, min_time = 0, LRU_hit = 0, LFU_hit = 0;
    for (i = 0; i < *rec_tb_len; i++){
        if (strcmp(rec_table[i].url, url) == 0) {
            rec_table[i].freq++;
            rec_table[i].time = time;
            break;
        }
    }
    if (i == *rec_tb_len){
        rec_table[i].url = url;
        rec_table[i].freq = 1;
        rec_table[i].time = time;
        *rec_tb_len = *rec_tb_len + 1;
    }
    // Search LFU cache
    for (j = 0; j < 3; j++){
        if (lfu[j].freq == 0 || strcmp(lfu[j].url, url) == 0) {
            lfu[j].time = rec_table[i].time;
            lfu[j].freq = rec_table[i].freq;
            lfu[j].url = rec_table[i].url;
            lfu[j].body = body;
            LFU_hit = 1;
            break;
        }
        if (lfu[j].freq < lfu[min_freq].freq){
            min_freq = j;
        }
    }
    // Search LRU cache
    for (j = 0; j < 1000; j++){
        if (lru[j].freq == 0 || strcmp(lru[j].url, url) == 0) {
            lru[j].time = rec_table[i].time;
            lru[j].freq = rec_table[i].freq;
            lru[j].url = rec_table[i].url;
            lru[j].body = body;
            LRU_hit = 1;
            break;
        }
        if (lru[j].time < lru[min_time].time){
            min_time = j;
        }
    }
    // Update LFU cache
    if (lfu[min_freq].freq <= rec_table[i].freq && LFU_hit == 0){
        lfu[min_freq].url = rec_table[i].url;
        lfu[min_freq].freq = rec_table[i].freq;
        lfu[min_freq].time = rec_table[i].time;
        lfu[min_freq].body = body;
        return; // No need to update LRU cache since the object is now in LFU
    }
    // Update LRU cache
    if (lru[min_time].time <= rec_table[i].time && LRU_hit == 0){
        lru[min_time].url = rec_table[i].url;
        lru[min_time].freq = rec_table[i].freq;
        lru[min_time].time = rec_table[i].time;
        lru[min_time].body = body;
    }
}


/**
 * Print LFU and LRU for debugging purpose.
 */
void disp(record *rec_table, entry *lfu, entry *lru, int rec_tb_len){
    printf("\n -------------------------------------------------------------- \n");
    for(int i = 0; i < 3; i++){
        if (lfu[i].freq == 0)
            break;
        printf("[lfu] i = %d, entry = [url: %s, body = %s, freq: %d, time = %d]\n", i, lfu[i].url, lfu[i].body, lfu[i].freq, lfu[i].time);
    }
    for(int i = 0; i < 1000; i++){
        if (lru[i].freq == 0)
            break;
        printf("[lru] i = %d, entry = [url: %s, body = %s, freq: %d, time = %d]\n", i, lru[i].url, lru[i].body, lru[i].freq, lru[i].time);
    }
    for(int i = 0; i < rec_tb_len; i++){
        printf("[rec] i = %d, entry = [url: %s, freq: %d, time = %d]\n", i, rec_table[i].url, rec_table[i].freq, rec_table[i].time);
    }
    printf("\n");
}

/**
 * Check if entry exists in LFU.
 */
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


/**
 * Check if entry exists in LRU.
 */
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