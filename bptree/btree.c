#include "btree.h"
#include <stdio.h>
#include <stdlib.h>

static inline int key_less(key_type a, key_type b) {
    return a < b ? 1 : 0; 
}

static inline int key_equal(key_type a, key_type b) {
    return a == b ? 1 : 0; 
}

static inline result_t btree_result(result_flags_t flags, key_type lastkey) {
    result_t r;
    r.flags = flags;
    r.lastkey = lastkey;
    return r;
}

static inline result_t brief_result(result_flags_t flags) {
    result_t r;
    r.flags = flags;
    return r;
}

static inline int result_has(result_t r, result_flags_t f) {
    return (r.flags & f) != 0 ? 1 : 0;
}

static inline result_t result_or (result_t r, result_t other) {
    r.flags = (r.flags | other.flags);

    if (result_has(other, btree_update_lastkey))
        r.lastkey = other.lastkey;

    return r;
}

static inline int is_inode_full(btree_t *bt, btree_inode_t *node) {
    return (node->slotuse == 2*bt->degree-1) ? 1 : 0;
}

static inline int is_inode_few(btree_t *bt, btree_inode_t *node) {
    return (node->slotuse <= bt->degree-1) ? 1 : 0;
}

static inline int is_inode_underflow(btree_t *bt, btree_inode_t *node) {
    return (node->slotuse < bt->degree-1) ? 1 : 0;
}

static inline int is_leaf_full(btree_t *bt, btree_fnode_t *node) {
    return (node->slotuse == 2*bt->degree-1) ? 1 : 0;
}

static inline int is_leaf_few(btree_t *bt, btree_fnode_t *node) {
    return (node->slotuse <= bt->degree-1) ? 1 : 0;
}

static inline int is_leaf_underflow(btree_t *bt, btree_fnode_t *node) {
    return (node->slotuse < bt->degree-1) ? 1 : 0;
}

static inline int find_lower_inner(btree_inode_t *node, key_type key) {
    int ix = 0;
    while (ix < node->slotuse && key_less(node->keyslots[ix], key))
        ++ix;
    return ix;
}

static inline int find_lower_leaf(btree_fnode_t *node, key_type key) {
    int ix = 0;
    while (ix < node->slotuse && key_less(node->keyslots[ix], key)) 
        ++ix;
    return ix;
}

static inline int node_slot_use(void *node, int level) {
    if (level == 0) return ((btree_fnode_t *)node)->slotuse;
    return ((btree_inode_t *)node)->slotuse;
}

static inline btree_fnode_t *allocate_leaf(int maxleafslots) {
    btree_fnode_t *node = malloc(sizeof(btree_fnode_t));
    if (node == NULL) {
        return NULL;
    }
    node->prevleaf = NULL;
    node->nextleaf = NULL;
    node->level = 0;
    node->slotuse = 0;
    node->keyslots = malloc(maxleafslots*sizeof(key_type));
    node->dataslots = malloc(maxleafslots*sizeof(data_type));
    return node;
}

static inline void free_leaf(btree_fnode_t *node) {
    free(node->keyslots);
    free(node->dataslots);
    free(node);
}

static inline btree_inode_t *allocate_inner(int maxinnerslots, unsigned short _level) {
    btree_inode_t *node = malloc(sizeof(btree_inode_t));
    if (node == NULL) {
        return NULL;
    }

    node->slotuse = 0;
    node->keyslots = malloc(maxinnerslots*sizeof(key_type));
    node->children = malloc((maxinnerslots+1)*sizeof(void *));
    node->level = _level;
    return node;
}

static inline void free_inode(btree_inode_t *node) {
    free(node->keyslots);
    free(node->children);
    free(node);
}

static inline void free_node(void *node, int level) {
    if (level == 0) {
        free_leaf(node);
    }
    free_inode(node);
}

static void split_leaf_node(btree_t *bt, btree_fnode_t *leaf, key_type *_newkey, void **_newleaf) {
    int i;
    unsigned int mid = (leaf->slotuse >> 1);
    btree_fnode_t *newleaf = allocate_leaf(2*bt->degree-1);
    newleaf->slotuse = leaf->slotuse - mid;
    newleaf->nextleaf = leaf->nextleaf;

    if (newleaf->nextleaf == NULL) {
        bt->ftail = newleaf;
    } else {
        newleaf->nextleaf->prevleaf = newleaf;
    }

    for (i = mid; i < leaf->slotuse; ++i) {
        newleaf->keyslots[i-mid] = leaf->keyslots[i];
        newleaf->dataslots[i-mid] = leaf->dataslots[i];
    }

    leaf->slotuse = mid;
    leaf->nextleaf = newleaf;
    newleaf->prevleaf = leaf;

    *_newkey = leaf->keyslots[leaf->slotuse - 1];
    *_newleaf = newleaf;
}

static void split_inner_node(btree_t *bt, btree_inode_t *inner, key_type *_newkey, void **_newinner, unsigned int addslot) {
    int i;
    unsigned int mid = (inner->slotuse >> 1);
    if (addslot <= mid && mid > inner->slotuse - (mid + 1))
        mid--;

    btree_inode_t *newinner = allocate_inner(2*bt->degree-1, inner->level);
    newinner->slotuse = inner->slotuse - (mid + 1);

    for (i = mid + 1; i < inner->slotuse; ++i) {
        newinner->keyslots[i-mid-1] = inner->keyslots[i];
        newinner->children[i-mid-1] = inner->children[i];
    }
    newinner->children[i-mid-1] = inner->children[i];

    inner->slotuse = mid;

    *_newkey = inner->keyslots[mid];
    *_newinner = newinner;
}

static int btree_insert_descend(btree_t *bt,
        void *node, unsigned short level, 
        key_type key, data_type value, 
        key_type *splitkey, void **splitnode) {
    int i, slot;

    if (level == 0) {
        btree_fnode_t *leaf = node;
        slot = find_lower_leaf(leaf, key);
        if (slot < leaf->slotuse && key_equal(key, leaf->keyslots[slot])) {
            // duplicated key, TODO
            return -1;
        }

        if (is_leaf_full(bt, leaf)) {
            split_leaf_node(bt, leaf, splitkey, splitnode);
            if (slot >= leaf->slotuse) {
                slot -= leaf->slotuse;
                leaf = *splitnode;
            }
        }

        for (i = leaf->slotuse - 1; i >= slot; --i) {
            leaf->keyslots[i+1] = leaf->keyslots[i];
            leaf->dataslots[i+1] = leaf->dataslots[i];
        }

        leaf->keyslots[slot] = key;
        leaf->dataslots[slot] = value;
        leaf->slotuse++;

        if (splitnode && leaf != *splitnode && slot == leaf->slotuse - 1)
        {
            *splitkey = key;
        }

        return 0;
    } else {
        key_type newkey;
        void *newchild = NULL;
        btree_inode_t *splitinner;
        btree_inode_t *inner = node;
        slot = find_lower_inner(inner, key);
        int r = btree_insert_descend(bt, inner->children[slot], inner->level-1,
                key, value, &newkey, &newchild);
        btree_inode_t *fxxk = bt->root;
        if (newchild) {
            if (is_inode_full(bt, inner)) {
                split_inner_node(bt, inner, splitkey, splitnode, slot);
                if (slot == inner->slotuse + 1 && inner->slotuse < ((btree_inode_t *)*splitnode)->slotuse) {
                    splitinner = *splitnode;
                    inner->keyslots[inner->slotuse] = *splitkey;
                    inner->children[inner->slotuse + 1] = splitinner->children[0];
                    inner->slotuse++;
                    splitinner->children[0] = newchild;
                    *splitkey = newkey;

                    return r;
                } else if (slot >= inner->slotuse + 1) {
                    slot -= inner->slotuse + 1;
                    inner = *splitnode;
                }
            }

            for (i = inner->slotuse - 1; i >= slot; --i) {
                inner->keyslots[i+1] = inner->keyslots[i];
            }
            for (i = inner->slotuse; i >= slot; --i) {
                inner->children[i+1] = inner->children[i];
            }

            inner->keyslots[slot] = newkey;
            inner->children[slot + 1] = newchild;
            inner->slotuse++;
        }
        return r;
    }
}

int btree_insert(btree_t *bt, key_type key, data_type value) {
    void *newchild = NULL;
    key_type newkey;
    int r;
    if (bt->root == NULL) {
        bt->root = allocate_leaf(2*bt->degree-1);
        if (bt->root == NULL) return -1;
        bt->fhead = bt->root;
        bt->ftail = bt->root;
        bt->height = 1;
    }
btree_inode_t *fxxk = bt->root;
    r = btree_insert_descend(bt, bt->root, bt->height-1, key, value,
            &newkey, &newchild);
    if (newchild) {
        btree_inode_t *newroot;
        newroot = allocate_inner(2*bt->degree-1, bt->height);
        newroot->keyslots[0] = newkey;
        newroot->children[0] = bt->root;
        newroot->children[1] = newchild;
        newroot->slotuse = 1;
        bt->root = newroot;
        bt->height++;
    }

    return r;
}

static result_t merge_leaves(btree_t *bt, btree_fnode_t *left,
        btree_fnode_t *right, btree_inode_t *parent) {
    int i;
    for (i = 0; i < right->slotuse; ++i) {
        left->keyslots[left->slotuse + i] = right->keyslots[i];
        left->dataslots[left->slotuse + i] = right->dataslots[i];
    }

    left->slotuse += right->slotuse;
    left->nextleaf = right->nextleaf;

    if (left->nextleaf)
        left->nextleaf->prevleaf = left;
    else
        bt->ftail = left;

    right->slotuse = 0;
    return brief_result(btree_fixmerge);
}

static result_t merge_inner(btree_inode_t *left, btree_inode_t *right,
        btree_inode_t *parent, unsigned int parentslot) {
    int i;
    left->keyslots[left->slotuse] = parent->keyslots[parentslot];
    left->slotuse++;

    for (i = 0; i < right->slotuse; ++i) {
        left->keyslots[left->slotuse + i] = right->keyslots[i];
        left->children[left->slotuse + i] = right->children[i];
    }
    left->children[left->slotuse + i] = right->children[i];

    left->slotuse += right->slotuse;
    right->slotuse = 0;

    return brief_result(btree_fixmerge);
}

static result_t shift_left_leaf(btree_fnode_t* left, btree_fnode_t* right,
        btree_inode_t* parent, unsigned int parentslot) {
    int i;
    unsigned int shiftnum = (right->slotuse - left->slotuse) >> 1;

    for (i = 0; i < shiftnum; ++i) {
        left->keyslots[left->slotuse + i] = right->keyslots[i];
        left->dataslots[left->slotuse + i] = right->dataslots[i];
    }

    left->slotuse += shiftnum;

    for (i = shiftnum; i < right->slotuse; ++i) {
        right->keyslots[i-shiftnum] = right->keyslots[i];
        right->dataslots[i-shiftnum] = right->dataslots[i];
    }

    right->slotuse -= shiftnum;

    if (parentslot < parent->slotuse) {
        parent->keyslots[parentslot] = left->keyslots[left->slotuse - 1];
        return brief_result(btree_ok);
    }
    else {
        return btree_result(btree_update_lastkey, left->keyslots[left->slotuse - 1]);
    }
}

static void shift_left_inner(btree_inode_t* left, btree_inode_t* right,
        btree_inode_t* parent, unsigned int parentslot) {
    int i;
    unsigned int shiftnum = (right->slotuse - left->slotuse) >> 1;

    left->keyslots[left->slotuse] = parent->keyslots[parentslot];
    left->slotuse++;

    for (i = 0; i < shiftnum - 1; ++i) {
        left->keyslots[left->slotuse + i] = right->keyslots[i];
        left->children[left->slotuse + i] = right->children[i];
    }
    left->children[left->slotuse + i] = right->children[i];

    left->slotuse += shiftnum - 1;
    parent->keyslots[parentslot] = right->keyslots[shiftnum - 1];

    for (i = shiftnum; i < right->slotuse; ++i) {
        right->keyslots[i-shiftnum] = right->keyslots[i];
        right->children[i-shiftnum] = right->children[i];
    }
    right->children[i-shiftnum] = right->children[i];

    right->slotuse -= shiftnum;
}

static void shift_right_leaf(btree_fnode_t* left, btree_fnode_t* right,
        btree_inode_t* parent, unsigned int parentslot) {
    int i;
    unsigned int shiftnum = (left->slotuse - right->slotuse) >> 1;

    for (i = right->slotuse - 1; i >= 0; ++i) {
        right->keyslots[i+shiftnum] = right->keyslots[i];
        right->dataslots[i+shiftnum] = right->dataslots[i];
    }
    right->slotuse += shiftnum;

    for (i = 0; i < shiftnum; ++i) {
        right->keyslots[i] = left->keyslots[left->slotuse-shiftnum+i];
        right->dataslots[i] = left->dataslots[left->slotuse-shiftnum+i];
    }

    left->slotuse -= shiftnum;
    parent->keyslots[parentslot] = left->keyslots[left->slotuse - 1];
}

static void shift_right_inner(btree_inode_t* left, btree_inode_t* right, 
        btree_inode_t* parent, unsigned int parentslot) {
    int i;
    unsigned int shiftnum = (left->slotuse - right->slotuse) >> 1;

    for (i = right->slotuse - 1; i >= 0; ++i) {
        right->keyslots[i+shiftnum] = right->keyslots[i];
        right->children[i+shiftnum+1] = right->children[i+1];
    }
    right->children[i+shiftnum+1] = right->children[i+1];

    right->slotuse += shiftnum;
    right->keyslots[shiftnum - 1] = parent->keyslots[parentslot];

    for (i = 0; i < shiftnum - 1; ++i) {
        right->keyslots[i] = left->keyslots[left->slotuse-shiftnum+i+1];
        right->children[i] = left->children[left->slotuse-shiftnum+i+1];
    }
    right->children[i] = left->children[left->slotuse-shiftnum+i+1];

    parent->keyslots[parentslot] = left->keyslots[left->slotuse - shiftnum];

    left->slotuse -= shiftnum;
}

result_t btree_erase_descend(btree_t *bt,
        key_type key,
        void* curr, int level,
        void* left, void* right,
        btree_inode_t* leftparent, btree_inode_t* rightparent,
        btree_inode_t* parent, unsigned int parentslot) {
    if (level == 0) {
        int i;
        btree_fnode_t *leaf =curr;
        btree_fnode_t *leftleaf = left;
        btree_fnode_t *rightleaf = right;

        int slot = find_lower_leaf(leaf, key);
        if (slot >= leaf->slotuse || !key_equal(key, leaf->keyslots[slot]))
        {
            return brief_result(btree_not_found);
        }

        for (i = slot + 1; i < leaf->slotuse; ++i) {
            leaf->keyslots[i-1] = leaf->keyslots[i];
            leaf->dataslots[i-1] = leaf->dataslots[i];
        }
        leaf->slotuse--;

        result_t myres = brief_result(btree_ok);
        
        if (slot == leaf->slotuse) {
            if (parent == NULL) {
                BTREE_ASSERT(leaf == bt->root);
            }
            else if (parentslot < parent->slotuse) {
                if (leaf->slotuse > 0) {
                    parent->keyslots[parentslot] = leaf->keyslots[leaf->slotuse - 1];
                }
            } else {
                if (leaf->slotuse > 0 ) {
                    myres = result_or(myres, btree_result(btree_update_lastkey, 
                                leaf->keyslots[leaf->slotuse - 1]));
                } else {
                    myres = result_or(myres, btree_result(btree_update_lastkey, leftleaf->keyslots[leftleaf->slotuse - 1]));
                    //diff
                    //BTREE_ASSERT(leaf == bt->root);
                }
            }
        }
        

        if (is_leaf_underflow(bt, leaf)) {
            if (leaf == bt->root) {
                if (leaf->slotuse >= 1) {
                    return brief_result(btree_ok);;  
                } else {
                    leaf = NULL;
                    //free_node(bt->root, bt->height-1);
                    bt->height = 0;
                    bt->root = NULL;
                    bt->fhead = NULL;
                    bt->ftail = NULL;

                    return brief_result(btree_ok);
                }
            }
            else if ((leftleaf == NULL || is_leaf_few(bt, leftleaf)) 
                    && (rightleaf == NULL || is_leaf_few(bt, rightleaf))) {
                if (leftparent == parent)
                    myres = result_or(myres, merge_leaves(bt, leftleaf, leaf, leftparent));
                else
                    myres = result_or(myres, merge_leaves(bt, leaf, rightleaf, rightparent));
            }
            else if ((leftleaf != NULL && is_leaf_few(bt, leftleaf)) 
                    && (rightleaf != NULL && !is_leaf_few(bt, rightleaf))) {
                if (rightparent == parent)
                    myres = result_or(myres, shift_left_leaf(leaf, rightleaf, rightparent, parentslot));
                else
                    myres = result_or(myres, merge_leaves(bt, leftleaf, leaf, leftparent));
            }
            else if ((leftleaf != NULL && !is_leaf_few(bt, leftleaf)) 
                    && (rightleaf != NULL && is_leaf_few(bt, rightleaf))) {
                if (leftparent == parent)
                    shift_right_leaf(leftleaf, leaf, leftparent, parentslot - 1);
                else
                    myres = result_or(myres, merge_leaves(bt, leaf, rightleaf, rightparent));
            }
            else if (leftparent == rightparent) {
                if (leftleaf->slotuse <= rightleaf->slotuse)
                    myres = result_or(myres, shift_left_leaf(leaf, rightleaf, rightparent, parentslot));
                else
                    shift_right_leaf(leftleaf, leaf, leftparent, parentslot - 1);
            }
            else {
                if (leftparent == parent)
                    shift_right_leaf(leftleaf, leaf, leftparent, parentslot - 1);
                else
                    myres = result_or(myres, shift_left_leaf(leaf, rightleaf, rightparent, parentslot));
            }
        }

        return myres;
    } else {
        int i;
        btree_inode_t* myleftparent, * myrightparent;
        btree_inode_t *inner = curr;
        btree_inode_t *leftinner = left;
        btree_inode_t *rightinner = right;
        void *myleft;
        void *myright;

        int slot = find_lower_inner(inner, key);
        if (slot == 0) {
            myleft = (left == NULL) ? NULL : ((btree_inode_t*)left)->children[((btree_inode_t*)left)->slotuse];
            // diff
            myleftparent = left;
        }
        else {
            myleft = inner->children[slot - 1];
            myleftparent = inner;
        }

        if (slot == inner->slotuse) {
            myright = (right == NULL) ? NULL : ((btree_inode_t*)right)->children[0];
            // diff
            myrightparent = right;
        }
        else {
            myright = inner->children[slot + 1];
            myrightparent = inner;
        }

        result_t result = btree_erase_descend(bt, key,
                inner->children[slot], level-1, 
                myleft, myright,
                myleftparent, myrightparent,
                inner, slot);

        result_t myres = brief_result(btree_ok);
        if (result_has(result, btree_not_found)) {
            return result;
        }
        btree_fnode_t *fxxk = inner->children[slot];
        //dump_node(fxxk, level-1);
        if (result_has(result, btree_update_lastkey)) {
            if (parent && parentslot < parent->slotuse) {
                parent->keyslots[parentslot] = result.lastkey;
            } else {
                myres = result_or(myres, btree_result(btree_update_lastkey, result.lastkey));
            }
        }

        if (result_has(result, btree_fixmerge)) {
            if (node_slot_use(inner->children[slot], inner->level-1) != 0)
                slot++;
            //free_node(inner->children[slot], inner->level - 1);
            //dump_node(fxxk, level-1);
            for (i = slot; i < inner->slotuse; ++i) {
                inner->keyslots[i-1] = inner->keyslots[i];
                inner->children[i] = inner->children[i+1];
            }
            inner->slotuse--;

            if (inner->level == 1)
            {
                slot--;
                btree_fnode_t *child = inner->children[slot];
                inner->keyslots[slot] = child->keyslots[child->slotuse - 1];
            }
        }

        if (is_inode_underflow(bt, inner) && !(inner == bt->root && inner->slotuse >= 1)) {
            if (leftinner == NULL && rightinner == NULL) {
                bt->root = inner->children[0];
                bt->height -= 1;
                inner->slotuse = 0;
                //free_node(inner, level);

                return brief_result(btree_ok);
            } else if ((leftinner == NULL || is_inode_few(bt, leftinner)) 
                    && (rightinner == NULL || is_inode_few(bt, rightinner))) {
                if (leftparent == parent)
                    myres = result_or(myres, merge_inner(leftinner, inner, leftparent, parentslot - 1));
                else
                    myres = result_or(myres, merge_inner(inner, rightinner, rightparent, parentslot));
            } else if ((leftinner != NULL && is_inode_few(bt, leftinner)) 
                    && (rightinner != NULL && !is_inode_few(bt, rightinner))) {
                if (rightparent == parent)
                    shift_left_inner(inner, rightinner, rightparent, parentslot);
                else
                    myres = result_or(myres, merge_inner(leftinner, inner, leftparent, parentslot - 1));
            } else if ((leftinner != NULL && !is_inode_few(bt, leftinner)) 
                    && (rightinner != NULL && is_inode_few(bt, rightinner))) {
                if (leftparent == parent)
                    shift_right_inner(leftinner, inner, leftparent, parentslot - 1);
                else
                    myres = result_or(myres, merge_inner(inner, rightinner, rightparent, parentslot));
            } else if (leftparent == rightparent) {
                if (leftinner->slotuse <= rightinner->slotuse)
                    shift_left_inner(inner, rightinner, rightparent, parentslot);
                else
                    shift_right_inner(leftinner, inner, leftparent, parentslot - 1);
            } else {
                if (leftparent == parent)
                    shift_right_inner(leftinner, inner, leftparent, parentslot - 1);
                else
                    shift_left_inner(inner, rightinner, rightparent, parentslot);
            }
        }

        return myres;
    }
}

int btree_erase(btree_t *bt, key_type key) {
    if (!bt->root) return -1;

    result_t result = btree_erase_descend(bt, key, bt->root, bt->height - 1, NULL, NULL, NULL, NULL, NULL, 0);

    return result_has(result, btree_not_found);
}

btree_t *btree_create(unsigned short degree) {
    btree_t *bt = malloc(sizeof(btree_t));
    BTREE_ASSERT(bt != NULL);
    bt->degree = degree;

    bt->root = allocate_leaf(2*bt->degree-1);
    BTREE_ASSERT(bt->root != NULL);
    bt->fhead = bt->root;
    bt->ftail = bt->root;
    bt->height = 1;

    return bt;
}

data_type btree_search(btree_t *bt, key_type key) {
    int slot;
    void* n = bt->root;
    btree_inode_t *inner;
    btree_fnode_t *leaf;
    unsigned short level = bt->height - 1;
    if (!n) return NULL;

    while (level > 0) {
        inner = n;
        slot = find_lower_inner(inner, key);
        n = inner->children[slot];
        level--;
    }

    leaf = n;
    slot = find_lower_leaf(leaf, key);
    return (slot < leaf->slotuse && key_equal(key, leaf->keyslots[slot]))
        ? leaf->dataslots[slot] : NULL;
}

void dump_node(void *n, int level) {
    btree_fnode_t *leaf;
    btree_inode_t *inner;
    void *subnode;
    int slot;
    if (level == 0) {
        leaf = n;
        printf("--------------------\n");
        printf("|      level=%d     |\n", leaf->level);
        printf("--------------------\n|");
        for (slot = 0; slot < leaf->slotuse; ++slot)
        {
            printf("  %d  |", leaf->keyslots[slot]);
        }
        printf("\n--------------------\n");
    } else {
        inner = n;
        printf("--------------------\n");
        printf("|      level=%d     |\n", inner->level);
        printf("--------------------\n|");
        for (slot = 0; slot < inner->slotuse; ++slot)
        {
            printf("  %d  |", inner->keyslots[slot]);
        }
        printf("\n--------------------\n");
        for (slot = 0; slot <= inner->slotuse; ++slot)
        {
            subnode = inner->children[slot];
            dump_node(subnode, level - 1);
        }
    }
}


