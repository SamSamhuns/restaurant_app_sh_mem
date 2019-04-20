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
#include <semaphore.h>
#include "common.h"

int main(int argc, char const *argv[]) {
	/* cmd args validation
	    ./server -m shmid */
	if ( argc != 3 || strcmp(argv[1], "-m") != 0 || isdigit_all(argv[2], strlen(argv[2]))) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./server -m shmid\n");
		return 1;
	}

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
	shm_fd = shm_open(argv[2], O_RDWR, 0666);
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

	/* if shutting down has already been initiated by the coordinator */
	if (shared_mem_ptr->initiate_shutdown == 1) {
		/* Clean up normally */
		all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
		return 0;
	}
	/* Check if more than one servers are being tried to be initiated */
	/* add Server pid to the server pid var in shared mem */
	if (shared_mem_ptr->server_pid == NO_SERVER_TEMP_PID) {
			////////* Acquire semaphore lock first before writing in shared memory *//////
			if (sem_wait(shared_mem_write_sem) == -1) {																	//
				perror("sem_wait()");																											//
				exit(1);																																	//
			}																																						//
			shared_mem_ptr->server_pid = getpid();																			//
																																									//
			/* release semaphore write lock after writing to shared memory */						//
			if (sem_post(shared_mem_write_sem) == -1) {																	//
				perror("sem_post()");																											//
				exit(1);																																	//
			}																																						//
			//////////////////////////////////////////////////////////////////////////////

	}
	else {
			printf("Only one server is allowed in the restaurant\n");
			/* Clean up normally */
			all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
			return 0;
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

		/*TODO handle the server queue */
	}
	// exit of main while loop

	/* WARNING Control should never reach here */
	fprintf(stderr, "Error: CONTROL escaped normal loop\n");
	return 1;
}
