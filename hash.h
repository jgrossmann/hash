#ifndef __HASH_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

struct node {
	char* string;
	struct node* next; 
	struct node* previous;
	int *nextstat;
	int *prevstat;
	int *stringstat;
	pthread_mutex_t *mutex_R;
	pthread_mutex_t *mutex_W;
};

typedef struct {
	struct node** htable;
	int size;

} hashtable_t;


// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int);

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *);

// add a new string to the hashtable
void hashtable_add(hashtable_t *, const char *);

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *, const char *);

// print the contents of the hashtable
void hashtable_print(hashtable_t *);

#endif

