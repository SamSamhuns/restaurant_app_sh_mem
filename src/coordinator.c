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

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *MaxCashiers, long *MaxPeople, long *MaxTimeWait);

int main(int argc, char const *argv[]) {
	long MaxCashiers, MaxPeople, MaxTimeWait;

	/* cmd args validation
	    ./coordinator -n MaxNumOfCashiers -p MaxPeople -t MaxTimeWait*/
	if (cmd_validate(argc, argv, &MaxCashiers, &MaxPeople, &MaxTimeWait) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./coordinator -n MaxNumOfCashiers -p MaxPeople -t MaxTimeWait\n");
		return 1;
	}
	// printf("DEBUG mc is %li, mp is %li and mt is %li\n", MaxCashiers, MaxPeople, MaxTimeWait);

	/* loading the menu items from the txt file into a menu_items struct*/
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);
	// if (DEBUG) {
	// 	int temp_num = 15;
	// 	printf("%li %s %li %li %li\n", menu_items[temp_num].menu_itemId,
	// 	       menu_items[temp_num].menu_desc, menu_items[temp_num].menu_price,
	// 	       menu_items[temp_num].menu_min_time, menu_items[temp_num].menu_max_time
	// 	       );
	// }



	/* name of the shared memory object */
	const char* name = "OS";

	/* strings written to shared memory */
	const char* message_0 = "Hello";
	const char* message_1 = "World!\n";

	/* shared memory file descriptor */
	int shm_fd;

	/* pointer to shared memory obect */
	void* ptr;

	/* create the shared memory object O_EXCL flag gives error if a shared memory
	    with the given name already exists */
	shm_fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, 0666);

	/* configure the size of the shared memory object */
	ftruncate(shm_fd, MAX_SHM_SIZE);

	/* memory map the shared memory object */
	ptr = mmap(0, MAX_SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);

	/* write to the shared memory object */
	sprintf(ptr, "%s", message_0);
	ptr += strlen(message_0);
	sprintf(ptr, "%s", message_1);
	ptr += strlen(message_1);

	int cc;
	printf("Scanning an int for syn\n");
	scanf("%d\n",&cc);

	munmap(ptr, MAX_SHM_SIZE);
	close(shm_fd);
	shm_unlink(name);


	/*TODO*/
	// initiate shared memory segment (shm)
	// initiate named semaphores
	//
	// init cur_n_cashiers // must be less than MaxNumOfCashiers
	// init cur_n_clients_wait_cashier // must be less than MaxPeople
	// init cur_n_clients_wait_server

	// init cashier_service_queue // len of queue equals cur_n_clients_wait_cashier
	// init server_service_queue // len of queue equals cur_n_clients_wait_server
	//
	// save MaxNumOfCashiers in shm // accessed by cashier
	// save MaxPeople in shm // accessed by client
	// save MaxTimeWait in shm // accessed by all
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
int cmd_validate(int argc, char const *argv[], long *MaxCashiers, long *MaxPeople, long *MaxTimeWait) {
	int maxC_found = 0;
	int maxP_found = 0;
	int maxT_found = 0;

	if ( argc != 7 ) {
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
		else if (strcmp(argv[i], "-t") == 0) {
			if (isdigit_all(argv[i+1], strlen(argv[i+1]))) {
				return 1;
			}
			maxT_found += 1;
			*MaxPeople = strtol(argv[i+1], NULL, 10);
		}
	}
	if (maxC_found == 1 && maxP_found == 1 && maxT_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
