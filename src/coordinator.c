#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

int main(int argc, char const *argv[]) {
	/* cmd args validation
		./coordinator -n MaxNumOfCashiers	*/
	if ( argc != 3 ||
		strcmp(argv[1], "-n") != 0 ||
		isdigit_all(argv[2], strlen(argv[2]))) {
		fprintf(stderr,
			"Incorrect args supplied. Usage: ./coordinator -n MaxNumOfCashiers\n");
		return 1;
	}
	long MaxCashiers;
	MaxCashiers = strtol(argv[2], NULL, 10);
	printf("DEBUG Long is %li \n", MaxCashiers);


	return 0;
}
