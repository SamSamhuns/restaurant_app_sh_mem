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
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);
	// if (DEBUG) {
	//  int temp_num = 15;
	//  printf("%li %s %li %li %li\n", menu_items[temp_num].menu_itemId,
	//         menu_items[temp_num].menu_desc, menu_items[temp_num].menu_price,
	//         menu_items[temp_num].menu_min_time, menu_items[temp_num].menu_max_time
	//         );
	// }

	/* shared memory file descriptor */
	int shm_fd;
	struct shared_memory_struct *shared_mem_ptr;

	sem_t *cashierS = sem_open(CASHIER_SEM, O_CREAT | O_EXCL, 0666, 0); /* Init cashierS semaphore to 0*/
	TRY_AND_CATCH_SEM(cashierS, "sem_open()");
	sem_t *clientQS = sem_open(CLIENTQ_SEM, O_CREAT | O_EXCL, 0666, 0); /* Init clientQS sempaphore to 0*/
	TRY_AND_CATCH_SEM(clientQS, "sem_open()");

	/* name of the shared memory object / shmid */
	const char* shmid = "0001";
	fprintf(stdout, "Shared mem restaurant id is %s\n", shmid);

	/* create the shared memory object O_EXCL flag gives error if a shared memory
	    with the given name already exists */
	shm_fd = shm_open(shmid, O_CREAT | O_RDWR | O_EXCL, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* configure the size of the shared memory object */
	if (ftruncate(shm_fd, sizeof(shared_memory_struct)) == -1 )
	{
		perror("ftruncate");
		exit(1);
	}
	printf("%zi\n",sizeof(shared_memory_struct) );

	/* memory map the shared memory object */
	if ((shared_mem_ptr = mmap(0, sizeof(shared_memory_struct),
	                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	};

	/* write to the shared memory object */
	shared_mem_ptr->MaxCashiers = MaxCashiers;
	shared_mem_ptr->MaxPeople = MaxPeople;

	// sprintf(ptr, "%s", message_0);
	// ptr += strlen(message_0);
	// sprintf(ptr, "%s", message_1);
	// ptr += strlen(message_1);

	int cc;
	printf("Scanning an int for syn\n");
	scanf("%d",&cc);


	/*EXIT CLEAN UP*/
	/* close the named semaphores */
	sem_close(cashierS);
	sem_close(clientQS);
	/* remove the named semaphores
	    IMPORTANT only coordinator should call this
	    at the end */
	sem_unlink(CASHIER_SEM);
	sem_unlink(CLIENTQ_SEM);

	munmap(shared_mem_ptr, MAX_SHM_SIZE);  /* unmap the shared mem object */
	close(shm_fd);  /* close the file descrp from shm_open */
	/* remove the shared mem object
	    IMPORTANT only coordinator should call this
	    at the end */
	shm_unlink(shmid);


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
