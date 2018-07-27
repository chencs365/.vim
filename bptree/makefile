a.out : btree_test.o btree.o
	cc -o a.out btree_test.o btree.o

btree_test.o : btree_test.c btree.h
	cc -g -c btree_test.c

btree.o : btree.c btree.h
	cc -g -D_GNU_SOURCE -c btree.c

clean :
	rm a.out btree_test.o btree.o
