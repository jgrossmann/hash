#include "hash.h"


//creates hash index for string
int getHash(const char* string, int tablesize,char type,hashtable_t *hashtable) {
   int M = 31, len = strlen(string), i = 0;
	unsigned long long hash = 1;
   for(; i < len; ++i) {
      hash =(unsigned long long) M * hash + string[i];
	}
  	hash = hash % tablesize;
	if(type == 'w') {
		pthread_mutex_lock(hashtable->htable[hash]->mutex_W);
		pthread_mutex_lock(hashtable->htable[hash]->mutex_R);
	}else if(type == 'r') {
		pthread_mutex_lock(hashtable->htable[hash]->mutex_R);
	}
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
		hashtable->htable[i] = malloc(sizeof(struct node));
		hashtable->htable[i]->stringstat = 0;
		hashtable->htable[i]->mutex_R = malloc(sizeof(pthread_mutex_t));
		hashtable->htable[i]->mutex_W = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(hashtable->htable[i]->mutex_R,NULL);
		pthread_mutex_init(hashtable->htable[i]->mutex_W,NULL);
	}
    return hashtable;
}

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	int i = 0;
	struct node* h;
	for(;i<hashtable->size;i++) {
		pthread_mutex_lock(hashtable->htable[i]->mutex_W);
		pthread_mutex_lock(hashtable->htable[i]->mutex_R);
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
		pthread_mutex_unlock(hashtable->htable[i]->mutex_W);
		pthread_mutex_unlock(hashtable->htable[i]->mutex_R);
		pthread_mutex_destroy(h->mutex_W); pthread_mutex_destroy(h->mutex_R);
		free(h->mutex_W); free(h->mutex_R);
		free(h);
	}
	free(hashtable->htable);
	free(hashtable);
}

// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {
	if(hashtable == NULL || s == NULL || hashtable->htable == NULL) {
		return;
	}
	int hash = getHash(s, hashtable->size,'r',hashtable);
	struct node* h = hashtable->htable[hash];
	if(!h->stringstat){
		h->string = malloc(sizeof(char)*strlen(s)+1);
		strcpy(h->string,s); h->nextstat =0; h->prevstat = 0; h->stringstat = 1;
	}else {
		h = hashtable->htable[hash];
		struct node* newnode = malloc(sizeof(struct node));
		newnode->string = malloc(sizeof(char)*strlen(s)+1);
		strcpy(newnode->string,s); 
		newnode->previous = NULL; newnode->stringstat = 1; newnode->nextstat = 1; newnode->prevstat = 0;
		newnode->mutex_R = malloc(sizeof(pthread_mutex_t));
		newnode->mutex_W = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(newnode->mutex_W,NULL); pthread_mutex_init(newnode->mutex_R,NULL);
		pthread_mutex_lock(newnode->mutex_R);
		newnode->next = h;hashtable->htable[hash] = newnode;
		h->previous = newnode; h->prevstat = 1;
		pthread_mutex_unlock(h->mutex_R);
		pthread_mutex_destroy(h->mutex_W); pthread_mutex_destroy(h->mutex_R);
		free(h->mutex_W); free(h->mutex_R);
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
	if(!h->stringstat) {
		pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
		pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
		return;
	}
	while(strcmp(h->string,s)){
		if(!h->nextstat){
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
			pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
			return;
		}
		h = h->next; 
	}
	if(!strcmp(h->string,s)) {
		if(!h->prevstat){
			if(!h->nextstat){
				free(h->string);
				h->stringstat = 0; h->next = NULL; h->previous = NULL;
				pthread_mutex_unlock(hashtable->htable[hash]->mutex_W);
				pthread_mutex_unlock(hashtable->htable[hash]->mutex_R);
				return;
			}
			h->next->mutex_R = malloc(sizeof(pthread_mutex_t));
			h->next->mutex_W = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(h->next->mutex_W,NULL); pthread_mutex_init(h->next->mutex_R,NULL);
			pthread_mutex_lock(h->next->mutex_W); pthread_mutex_lock(h->next->mutex_R);
			hashtable->htable[hash] = h->next;
			hashtable->htable[hash]->prevstat = 0;
			pthread_mutex_unlock(h->mutex_R); pthread_mutex_unlock(h->mutex_W);
			pthread_mutex_destroy(h->mutex_W); pthread_mutex_destroy(h->mutex_R);
			free(h->mutex_W); free(h->mutex_R);
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
}
int main(void) {
	hashtable_t *htable;
	printf("%d\n",sizeof(htable->htable));
	hashtable_add(htable, "hello");
	hashtable_add(htable, "hello");
	hashtable_add(htable, "there");
	hashtable_add(htable, "sir");
	hashtable_print(htable);
	hashtable_remove(htable, "hello");
	return 0;
}
