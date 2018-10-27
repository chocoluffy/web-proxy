#include <stdio.h>
#include <string.h>

typedef struct LFU_entry {
  char *url;
  char *body;
  int freq;
} lfu_entry;

typedef struct Record {
  char *url;
  int freq;
} record;

void update_LFU(char *url, char *body, record *rec_table, lfu_entry *lfu,
                 int *rec_tb_len);
int get_LFU(char *url, lfu_entry *lfu, char* res);

void update_LFU(char *url, char *body, record *rec_table, lfu_entry *lfu,
                 int *rec_tb_len) {
  /**
   * Update LFU:
   * 1. If the given URL appear in the cache, then just update the frequency
   * adding 1
   * 2. Otherwise, if the URL appeared before, increase its frequency by 1 and
   * then check whether it is frequent enough to be in the cache. If so, update
   * cache.
   * 3. If this is the first time this URL appears, add the URL to the record
   * table with frequency 1. In any of the cases, return the body (web content
   * associated with the URL)
   */
  int min_entry = 0;
  for (int i = 0; i < 3; i++) {
    if (lfu[i].freq == 0) {
      lfu[i].url = url;
      lfu[i].freq = 1;
      lfu[i].body = body;
    //   return body;
    } else {
      if (strcmp(lfu[i].url, url) == 0) {
        lfu[i].freq++;
        // return body;
      }
      if (lfu[i].freq < lfu[min_entry].freq) {
        min_entry = i;
      }
    }
  }
  for (int i = 0; i < *rec_tb_len; i++) {
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
    //   return body;
    }
  }
  rec_table[*rec_tb_len].url = url;
  rec_table[*rec_tb_len].freq = 1;
  *rec_tb_len = *rec_tb_len + 1;
//   return body;
}

int get_LFU(char *url, lfu_entry *lfu, char* res) {
    /**
     * return -1: content from this url is not cached in lfu table.
     */
    for (int i = 0; i < 3; i++) {
        if (lfu[i].freq == 0) return -1; // lfu_table has not been initialized.
        if (strcmp(url, lfu[i].url) == 0) {
            res = lfu[i].body;
            return 0;
        }
    }
    return -1;
}

// int main() {
  
//   lfu_entry lfu[3];
//   for(int i= 0; i< 3; i++) {
//       lfu[i].url = NULL;
//       lfu[i].freq = 0;
//       lfu[i].body = NULL;
//   }
//   record rec_table[1000];
//   int rec_tb_len = 0;
//   update_LFU("a.com", "aaaaa", rec_table, lfu, &rec_tb_len);
//   update_LFU("b.com", "bbbbb", rec_table, lfu, &rec_tb_len);
//   update_LFU("a.com", "aaaaa", rec_table, lfu, &rec_tb_len);
//   update_LFU("c.com", "ccc", rec_table, lfu, &rec_tb_len);
//   return 0;
// }