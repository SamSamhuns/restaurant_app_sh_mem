#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

int main(int argc, char const *argv[]) {
	/* cmd args validation
		./server -m shmid	*/
	if ( argc != 3 || strcmp(argv[1], "-m") != 0 || isdigit_all(argv[2], strlen(argv[2]))) {
		fprintf(stderr,
			"Incorrect args supplied. Usage: ./server -m shmid(number)\n");
		return 1;
	}
	long shmid;
	shmid = strtol(argv[2], NULL, 10);
	printf("DEBUG Long is %li \n", shmid);


	return 0;
}
