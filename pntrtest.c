#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "hash.h"

int main(void){
	pthread_mutex_t mutex1,*mutex2,*mutex3;
	pthread_mutex_init(&mutex1,NULL);
	pthread_mutex_lock(&mutex1);
	mutex2 = &mutex1;
	pthread_mutex_unlock(&mutex1);
	int c= pthread_mutex_trylock(mutex2);
	printf("%d\n",c);
	int a = 4;
	int* b = &a;
	printf("%d %d\n",a,*b);
	pthread_mutex_unlock(mutex2);
	mutex3 = mutex2;
	c= pthread_mutex_trylock(mutex3);
	printf("%d\n",c);
	pthread_mutex_unlock(mutex3);
	pthread_mutex_destroy(mutex3);
	return 0;
}
