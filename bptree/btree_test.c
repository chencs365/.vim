#include "btree.h"
#include <stdio.h>

int main() {
    int i;
    int j = 20;
    btree_fnode_t *ptr;
    btree_t *bt = btree_create(3);
    printf("bt->root : %p\n", bt->root);
    printf("bt->degree : %d\n", bt->degree);
    printf("bt->height : %d\n", bt->height);
    for (i = 1; i < 800; ++i) {
        btree_insert(bt, i, &j);
    }

    printf("bt->degree : %d\n", bt->degree);
    printf("bt->height : %d\n", bt->height);
    for (i = 1; i < 8000; ++i) {
        btree_erase(bt, i);
    }

    //btree_inode_t *i
   //dump_node(bt->root, bt->height-1);

    ptr = bt->fhead;
    while (ptr) {
        //dump_node(ptr, ptr->level);
        ptr = ptr->nextleaf;
    }
    return 0;
}
