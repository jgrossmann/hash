/* John Grossmann Project 2: Hash table threadsafe

My general structure is a bucketed hash table implemented as an array of linked lists.
In order to maximize concurrency while ensuring mutual exclusion, I put 2 locks in 
each head node of the linked list in each bucket. Whenever a function wants to go into
a specific bucket based on its hash, It must first check if the lock that that function
corresponds to is locked. This allows every other bucket in the table to be accessed 
concurrently.*/



#include "hash.h"

//this is used to prevent anything going into the hash table while its being freed.
pthread_mutex_t htable_mutex;

//generates hash index for string using the Kernighan and Ritchie multiplicative hash function.
int getHash(const char* string, int tablesize,char type,hashtable_t *hashtable) {
	pthread_mutex_lock(&htable_mutex);
   int M = 31, len = strlen(string), i = 0;
	unsigned long long hash = 0;
   for(; i < len; ++i) {
      hash =(unsigned long long) M * hash + string[i];
	}
  	hash = hash % tablesize;

//sets mutex locks on buckets based on variable 'type' given by calling function
	if(type == 'w') {
//indicates that the calling function does not want anyone to access the bucket
//started as w for write but now is just for the remove function. sets both
//the read and write mutex lock on the bucket to ensure exclusion.
		pthread_mutex_lock(hashtable->htable[hash]->mutex_W);
		pthread_mutex_lock(hashtable->htable[hash]->mutex_R);
	}else if(type == 'r') {
//indicates that the calling function is allowing the bucket to still be read.
//the add function activates this mutex to stop any other add/removes but
//allows prints.
		pthread_mutex_lock(hashtable->htable[hash]->mutex_R);
	}
	pthread_mutex_unlock(&htable_mutex);
	return (int) hash;
}



// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) {
	if(sizehint == 0) {
		return NULL;
	}
	hashtable_t *hashtable = malloc(sizeof(hashtable_t));
	hashtable->size = sizehint; hashtable->htable = malloc(sizeof(*hashtable->htable)*sizehint);
	int i = 0;
	for(;i<sizehint;i++){
//create all the buckets based on sizehint and allocate memory for the locks and initialize them
		hashtable->htable[i] = malloc(sizeof(struct node));
		hashtable->htable[i]->stringstat = 0;
		hashtable->htable[i]->mutex_R = malloc(sizeof(pthread_mutex_t));
		hashtable->htable[i]->mutex_W = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(hashtable->htable[i]->mutex_R,NULL);
		pthread_mutex_init(hashtable->htable[i]->mutex_W,NULL);
	}
	pthread_mutex_init(&htable_mutex,NULL);
    return hashtable;
}



// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	pthread_mutex_lock(&htable_mutex);
	int i = 0;
	struct node* h;
	for(;i<hashtable->size;i++) {
		h = hashtable->htable[i];
		if(h->stringstat){
			while(h->nextstat) {
				h = h->next;
			}
			while(h->prevstat) {
				h = h->previous; 
				free(h->next->string);free(h->next);
			}
			free(h->string);
		}
		pthread_mutex_destroy(h->mutex_W); pthread_mutex_destroy(h->mutex_R);
		free(h->mutex_W); free(h->mutex_R);
		free(h);
	}
	free(hashtable->htable);
	free(hashtable);
	pthread_mutex_unlock(&htable_mutex);
	pthread_mutex_destroy(&htable_mutex);
}



// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {
	if(hashtable == NULL || s == NULL || hashtable->htable == NULL) {
		return;
	}
	//gets hash of string and checks if bucket is locked
	int hash = getHash(s, hashtable->size,'r',hashtable);
	struct node* h = hashtable->htable[hash];
	if(!h->stringstat){ //if the bucket is empty
		h->string = malloc(sizeof(char)*strlen(s)+1);
		strcpy(h->string,s); h->nextstat =0; h->prevstat = 0; h->stringstat = 1;
	}else {//if the bucket is not empty
		h = hashtable->htable[hash];
		struct node* newnode = malloc(sizeof(struct node));
		newnode->string = malloc(sizeof(char)*strlen(s)+1);
		strcpy(newnode->string,s); 
		newnode->previous = NULL; newnode->stringstat = 1; newnode->nextstat = 1; newnode->prevstat = 0;
		newnode->mutex_R = h->mutex_R; newnode->mutex_W = h->mutex_W;
		newnode->next = h;hashtable->htable[hash] = newnode;
		h->previous = newnode; h->prevstat = 1;
	}
	pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
}

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *hashtable, const char *s) {
	if(hashtable == NULL || s == NULL || hashtable->htable == NULL) {
		return;
	}
	int hash = getHash(s, hashtable->size,'w',hashtable);
	struct node* h = hashtable->htable[hash];
	if(!h->stringstat) { //if the bucket the string should be in is empty
		pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
		pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
		return;
	}
	while(strcmp(h->string,s)){//while the strings are not a match
		if(!h->nextstat){
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
			return;
		}
		h = h->next; 
	}
	if(!strcmp(h->string,s)) {//if its the correct node
		if(!h->prevstat){//if first node in bucket
			if(!h->nextstat){//if its the only node in bucket
				free(h->string);
				h->stringstat = 0; h->next = NULL; h->previous = NULL;
				pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
				pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
				return;
			}
			h->next->mutex_R = h->mutex_R; h->next->mutex_W = h->mutex_W;
			hashtable->htable[hash] = h->next;
			hashtable->htable[hash]->prevstat = 0;
			free(h->string); free(h);
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
			return;	
		}
		if(h->nextstat){
			h->previous->next = h->next;
			h->next->previous = h->previous;
		}else if(h->prevstat){
			h->previous->next = NULL;
			h->previous->nextstat = 0;
		}
		free(h->string);
		free(h);
	}
	pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
	pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
}

// print the contents of the hashtable
void hashtable_print(hashtable_t *hashtable) {
	if(hashtable == NULL || hashtable->htable == NULL) {
		printf("There is no hastable, make one first.");
		return;
	}
	pthread_mutex_lock(&htable_mutex);
	int size = hashtable->size, i = 0;
	struct node* entry;
	for(;i<size;i++){
		pthread_mutex_lock(hashtable->htable[i]->mutex_W);
		entry = hashtable->htable[i];
		if(entry->stringstat) {
			while(entry->nextstat){
				printf("%s\n",entry->string);
				entry = entry->next;
			}
			printf("%s\n",entry->string);
		}
		pthread_mutex_unlock(hashtable->htable[i]->mutex_W);
	}
	pthread_mutex_unlock(&htable_mutex);
}
