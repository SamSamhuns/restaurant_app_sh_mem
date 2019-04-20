#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <semaphore.h>
#include "common.h"

/* cmd args validation
    ./cashier -s serviceTime -b breakTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *serviceTime, char *shmid, long *breakTime);

int main(int argc, char const *argv[]) {
	long serviceTime, breakTime;
	char shmid[MAX_SHMID_LEN];
	/* cmd args validation
	    ./cashier -s serviceTime -m shmid -b breakTime */
	if (cmd_validate(argc, argv, &serviceTime, shmid, &breakTime) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./cashier -s serviceTime -b breakTime -m shmid\n");
		return 1;
	}

	/* loading the menu items from the txt file into a menu_items struct*/
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);

	////////////////* ACCESS THE SHARED MEMORY STRUCTURE *//////////////////////
	sem_t *clientQS = sem_open(CLIENTQ_SEM, 0); /* open existing clientQS semaphore */
	sem_t *cashierS = sem_open(CASHIER_SEM, 0); /* open existing cashierS semaphore */
	sem_t *shared_mem_write_sem = sem_open(SHARED_MEM_WR_LOCK_SEM, 0); /* open existing shared_mem_write_sem semaphore */
	TRY_AND_CATCH_SEM(clientQS, "sem_open()");
	TRY_AND_CATCH_SEM(cashierS, "sem_open()");
	TRY_AND_CATCH_SEM(shared_mem_write_sem, "sem_open()");

	/* shared memory file descriptor */
	int shm_fd;
	struct Shared_memory_struct *shared_mem_ptr;

	/* open the shared memory object */
	shm_fd = shm_open(shmid, O_RDWR, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* memory map the shared memory object */
	if ((shared_mem_ptr = mmap(0, sizeof(Shared_memory_struct),
	                           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	};

	/* read from the shared memory object */
	printf("DEBUG The maxCashier number is %i\n",  shared_mem_ptr->MaxCashiers);
	printf("DEBUG The maxPeople number is %i\n", shared_mem_ptr->MaxPeople);


	/* if maximum number of cashiers have been reached */
	if ( (shared_mem_ptr->cur_cashier_num)+1 > shared_mem_ptr->MaxCashiers ) {
		printf("Cashier will not be joining restaurant as max num of cashiers exceeded\n");
		/* Clean up normally */
		all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
		return 0;
	}
	/* join the cashier list by adding cashier pid to cashier_pid_queue */
	else {
		/* Acquire semaphore lock first before writing */
		if (sem_wait(shared_mem_write_sem) == -1) {
			perror("sem_wait()");
			exit(1);
		}

		int cashier_pos_temp = shared_mem_ptr->cur_cashier_num;
		shared_mem_ptr->cashier_pid_queue[cashier_pos_temp] = getpid();
		shared_mem_ptr->cur_cashier_num += 1;

		/* release semaphore write lock after writing to shared memory */
		if (sem_post(shared_mem_write_sem) == -1) {
			perror("sem_post()");
			exit(1);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in Cashier begins *//////////////////////
	////////////////////////////////////////////////////////////////////////////

	while (1) {
		/* if shutting down has been initiated by the coordinator */
		if (shared_mem_ptr->initiate_shutdown == 1) {
			/* Clean up normally */
			all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
			return 0;
		}
	}
	// exit of main while loop

	// DEBUG
	int cc;
	printf("REMOVE LATER Scanning an int for TEMP SYNCHRONIZATION:\n");
	scanf("%d",&cc);
	// DEBUG END

	/* Clean up normally */
	all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
	return 0;
}

/* function for validating the cmd line args input */
int cmd_validate(int argc, char const *argv[], long *serviceTime, char *shmid, long *breakTime) {
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
			strcpy(shmid, argv[i+1]);
		}
	}
	if (serviceTime_found == 1 && shmid_found == 1 && breakTime_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}
