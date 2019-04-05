#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *MaxCashiers, long *MaxPeople);

int main(int argc, char const *argv[]) {
	long MaxCashiers, MaxPeople;

	/* cmd args validation
	    ./coordinator -n MaxNumOfCashiers -p MaxPeople*/
	if (cmd_validate(argc, argv, &MaxCashiers, &MaxPeople) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./coordinator -n MaxNumOfCashiers -p MaxPeople\n");
		return 1;
	}
	printf("DEBUG mc is %li, mp is %li\n", MaxCashiers, MaxPeople);


	/*TODO*/
	// initiate shared memory segment (shm)
	// initiate unnamed semaphores
	//
	// init cur_n_cashiers // must be less than MaxNumOfCashiers
	// init cur_n_clients_wait_cashier // must be less than MaxPeople
	// init cur_n_clients_wait_server

	// init cashier_service_queue // len of queue equals cur_n_clients_wait_cashier
	// init server_service_queue // len of queue equals cur_n_clients_wait_server
	//
	// save MaxNumOfCashiers in shm // accessed by cashier
	// save MaxPeople in shm // accessed by client
	// load menu items struct array structure and save in shm
	//
	//
	//
	// read client id, order and price from shm and save to db folder
	// gen summary statistics and save to stat folder
	//
	// deallocate semaphores
	// deallocate shm
	// exit

	return 0;
}

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *MaxCashiers, long *MaxPeople) {
	int maxC_found = 0;
	int maxP_found = 0;

	if ( argc != 5 ) {
		return 1;
	}

	for (int i = 1; i < argc-1; i++) {
		if (strcmp(argv[i], "-n") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			maxC_found += 1;
			*MaxCashiers = strtol(argv[i+1], NULL, 10);
		}
		else if (strcmp(argv[i], "-p") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			maxP_found += 1;
			*MaxPeople = strtol(argv[i+1], NULL, 10);
		}
	}
	if (maxC_found == 1 && maxP_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
