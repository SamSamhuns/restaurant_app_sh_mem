#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include "common.h"

/* cmd args validation
    ./client -i itemId -e eatTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *itemId, long *shmid, long *eatTime);

int main(int argc, char const *argv[]){
	long itemId, shmid, eatTime;

	/* cmd args validation
	    ./client -i itemId -e eatTime -m shmid	*/
	if (cmd_validate(argc, argv, &itemId, &shmid, &eatTime) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./client -i itemId(number) -e eatTime(number) -m shmid(number)\n");
		return 1;
	}
	printf("DEBUG i is %li, sh is %li, et is %li\n",itemId, shmid, eatTime );


	/*TODO*/
	// get shmid and access it
	// check cur_n_clients_wait_cashier against MaxPeople if not exit
	//
	// Add itself to the cashier_service_queue
	// deal with cashier in [1....cashier->serviceTime] save time stats in shm
	//
	// Add itself to the server_service_queue
	// deal/get food from server in [min_serv_time....max_serv_time] save time stats in shm
	// eat for [1....eatTime] save time stats in shm
	//
	// deal with semaphores P() and V() funcs
	// save its client_ID, item and price info in shm
	// log all time spent with cashier, server and eating in shm
	// exit


	return 0;
}

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *itemId, long *shmid, long *eatTime) {
	int itemId_found = 0;
	int shmid_found = 0;
	int eatTime_found = 0;

	if ( argc != 7 ) {
		return 1;
	}

	for (int i = 1; i < argc-1; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			itemId_found += 1;
			*itemId = strtol(argv[i+1], NULL, 10);
		}
		else if (strcmp(argv[i], "-e") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			eatTime_found += 1;
			*eatTime = strtol(argv[i+1], NULL, 10);
		}
		else if (strcmp(argv[i], "-m") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			shmid_found += 1;
			*shmid = strtol(argv[i+1], NULL, 10);
		}
	}
	if (itemId_found == 1 && shmid_found == 1 && eatTime_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
