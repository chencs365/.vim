#include "btree.h"
#include <stdio.h>

int main() {
    int i;
    int j = 20;
    btree_fnode_t *ptr;
    btree_t *bt = btree_create(10);
    printf("bt->root : %p\n", bt->root);
    printf("bt->degree : %d\n", bt->degree);
    printf("bt->height : %d\n", bt->height);
    for (i = 1; i < 300; ++i) {
        btree_insert(bt, i, &j);
    }

    printf("bt->degree : %d\n", bt->degree);
    printf("bt->height : %d\n", bt->height);
    for (i = 1; i < 4; ++i) {
        btree_erase(bt, i);
    }
   //dump_node(bt->root, bt->height-1);
   btree_erase(bt, 4);
   dump_node(bt->root, bt->height-1);

    /*
    ptr = bt->fhead;
    while (ptr) {
        dump_node(ptr, ptr->level);
        ptr = ptr->nextleaf;
    }
    */
    return 0;
}
