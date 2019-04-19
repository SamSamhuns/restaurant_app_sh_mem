#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <semaphore.h>
#include "common.h"

/* cmd args validation
    ./client -i itemId -e eatTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *itemId, char *shmid, long *eatTime);

int main(int argc, char const *argv[]){
	long itemId, eatTime;
	char shmid[MAX_SHMID_LEN];

	/* cmd args validation
	    ./client -i itemId -e eatTime -m shmid	*/
	if (cmd_validate(argc, argv, &itemId, shmid, &eatTime) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./client -i itemId -e eatTime -m shmid\n");
		return 1;
	}
	// printf("DEBUG i is %li, sh is %s, et is %li\n",itemId, shmid, eatTime );

	/* loading the menu items from the txt file into a menu_items struct*/
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);

	sem_t *clientQS = sem_open(CLIENTQ_SEM, 0); /* open existing clientQS semaphore */
	TRY_AND_CATCH_SEM(clientQS, "sem_open()");
	sem_t *cashierS = sem_open(CASHIER_SEM, 0); /* open existing cashierS semaphore */
	TRY_AND_CATCH_SEM(cashierS, "sem_open()");

	/* shared memory file descriptor */
	int shm_fd;

	/* pointer to shared memory object */
	void* ptr;

	/* open the shared memory object */
	shm_fd = shm_open(shmid, O_RDONLY, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* memory map the shared memory object */
	ptr = mmap(0, sizeof(shared_memory_struct),
	           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	int cc;
	printf("Scanning an int for syn\n");
	scanf("%d",&cc);
	/* read from the shared memory object */
	printf("%s", (char*)ptr);

	/* close the named semaphores */
	sem_close(clientQS);
	sem_close(cashierS);

	/* remove the shared memory object */
	munmap(ptr, MAX_SHM_SIZE);
	close(shm_fd);
	/* Cashiers should not delete the shared mem object
	    shm_unlink(shmid);*/

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
int cmd_validate(int argc, char const *argv[], long *itemId, char *shmid, long *eatTime) {
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
			strcpy(shmid, argv[i+1]);
		}
	}
	if (itemId_found == 1 && shmid_found == 1 && eatTime_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
