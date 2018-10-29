#include <stdio.h>
#include <string.h>
#include <pthread.h>

struct thread_info {    /* Used as argument to update() */
           pthread_t thread_id;        /* ID returned by pthread_create() */
           char*        table;
       };

void *update(void *arg) {
    struct thread_info *tinfo = arg;
    int row = (int)(tinfo->thread_id);
    printf("row id: %d\n", row);
    tinfo->table[row] = row +'0';
}

int main() {
    int n_thread = 3;
    char table[5];
    for (int i = 0; i < 5; i++) {
        table[i] = NULL;
    }
    struct thread_info *tinfo = calloc(n_thread, sizeof(struct thread_info));
    pthread_t *tid = malloc( n_thread * sizeof(pthread_t) );
    for(int i=0; i<n_thread; i++ ) {
        tinfo[i].thread_id = i;
        tinfo[i].table = table;
        pthread_create( &tid[i], NULL, &update, &tinfo[i]);
    }

    for(int i=0; i<n_thread; i++ ) {
        pthread_join( tid[i], NULL );
    }

    for(int i = 0; i < 5; i++) {
        printf("table row %d: %c\n.", i, table[i]);
    }

    return 0;
}