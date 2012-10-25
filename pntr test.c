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
	int a = 4;
	int* b = &a;
	printf("%d %d\n",a,b);
	return 0;
}
