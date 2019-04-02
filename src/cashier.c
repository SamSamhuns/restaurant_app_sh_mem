#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

/* cmd args validation
	./cashier -s serviceTime -b breakTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *serviceTime, long *shmid, long *breakTime);

int main(int argc, char const *argv[]) {
	long serviceTime, shmid, breakTime;
	/* cmd args validation
		./cashier -s serviceTime -m shmid -b breakTime */
	if (cmd_validate(argc, argv, &serviceTime, &shmid, &breakTime) == 1) {
		fprintf(stderr,
				"Incorrect args supplied. Usage: ./cashier -s serviceTime(number) -b breakTime(number) -m shmid(number)\n");
		return 1;
	}
	printf("s is %li, m is %li, b is %li\n",serviceTime, shmid, breakTime);

	return 0;
}

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *serviceTime, long *shmid, long *breakTime) {
	int serviceTime_found = 0;
	int shmid_found = 0;
	int breakTime_found = 0;

	if ( argc != 7 ) {
		return 1;
	}

	for (int i = 1; i < argc-1; i++) {
		if (strcmp(argv[i], "-s") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			serviceTime_found += 1;
			*serviceTime = strtol(argv[i+1], NULL, 10);
		}
		else if (strcmp(argv[i], "-b") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			breakTime_found += 1;
			*breakTime = strtol(argv[i+1], NULL, 10);
		}
		else if (strcmp(argv[i], "-m") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			shmid_found += 1;
			*shmid = strtol(argv[i+1], NULL, 10);
		}
	}
	if (serviceTime_found == 1 && shmid_found == 1 && breakTime_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
