#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

pthread_mutex_t mutex;
void *f1(void *threadarg){
	pthread_mutex_lock(&mutex);
	printf("f1\n");
	pthread_mutex_unlock(&mutex);
	printf("idk\n");
	return NULL;
}
void *f2(void *threadarg){
	pthread_mutex_lock(&mutex);
	printf("f2\n");
	pthread_mutex_unlock(&mutex);
	printf("why\n");
	return NULL;
}
int main(void){
	pthread_mutex_init(&mutex,NULL);
	pthread_t t1,t2;
	int a,b;
	a=pthread_create(&t1,NULL,f2,NULL);
	//b=pthread_create(&t2,NULL,f1,NULL);
	b= pthread_join(&t1,NULL);
	printf("waiting\n");
	pthread_mutex_destroy(&mutex);
	return 0;
}
