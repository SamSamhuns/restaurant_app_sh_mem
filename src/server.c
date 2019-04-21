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
	sem_t *server_sem = sem_open(SERVER_SEM, 0); /* open existing server_sem semaphore */
	sem_t *server_cli_q_sem = sem_open(SERVER_CLI_Q_SEM, 0); /* open existing server_cli_q_sem semaphore */
	sem_t *deq_s_block_sem = sem_open(DEQ_S_BLOCK_SEM, 0); /* open existing deq_s_block_sem semaphore */
	sem_t *shm_write_sem = sem_open(SHM_WRITE_SEM, 0); /* open existing server_sem semaphore */
	sem_t *shutdown_sem = sem_open(SHUTDOWN_SEM, 0); /* open existing server_sem semaphore */
	TRY_AND_CATCH_SEM(server_sem, "sem_open()");
	TRY_AND_CATCH_SEM(server_cli_q_sem, "sem_open()");
	TRY_AND_CATCH_SEM(deq_s_block_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shm_write_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shutdown_sem, "sem_open()");

	/* shared memory file descriptor */
	int shm_fd;
	struct Shared_memory_struct *shm_ptr;

	/* open the shared memory object */
	shm_fd = shm_open(argv[2], O_RDWR, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* memory map the shared memory object */
	if ((shm_ptr = mmap(0, sizeof(Shared_memory_struct),
	                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	};

	/* read from the shared memory object */
	printf("DEBUG The maxCashier number is %i\n",  shm_ptr->MaxCashiers);
	printf("DEBUG The maxPeople number is %i\n", shm_ptr->MaxPeople);

	/* if shutting down has already been initiated by the coordinator */
	if (shm_ptr->initiate_shutdown == 1) {
		/* Clean up normally */
		server_exit_cleanup(server_sem,
							server_cli_q_sem,
							deq_s_block_sem,
							shm_write_sem,
							shutdown_sem,
							shm_ptr,
							&shm_fd);
		return 0;
	}
	/* Check if more than one servers are being tried to be initiated */
	/* add Server pid to the server pid var in shared mem */
	if (shm_ptr->server_pid == NO_SERVER_TEMP_PID) {
		////////* Acquire semaphore lock first before writing in shared memory *////
		if (sem_wait(shm_write_sem) == -1) {                               //
			perror("sem_wait()");                                                 //
			exit(1);                                                              //
		}                                                                         //
		shm_ptr->server_pid = getpid();                                    //
		//
		/* release semaphore write lock after writing to shared memory */         //
		if (sem_post(shm_write_sem) == -1) {                               //
			perror("sem_post()");                                                 //
			exit(1);                                                              //
		}                                                                         //
		////////////////////////////////////////////////////////////////////////////

	}
	else {
		printf("Only one server is allowed in the restaurant\n");
		/* Clean up normally */
		server_exit_cleanup(server_sem,
							server_cli_q_sem,
							deq_s_block_sem,
							shm_write_sem,
							shutdown_sem,
							shm_ptr,
							&shm_fd);
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in Cashier begins *//////////////////////
	////////////////////////////////////////////////////////////////////////////

	while (1) {
		/* if shutting down has been initiated by the coordinator */
		if (shm_ptr->initiate_shutdown == 1) {
			/* Clean up normally */
			server_exit_cleanup(server_sem,
			                    server_cli_q_sem,
			                    deq_s_block_sem,
			                    shm_write_sem,
			                    shutdown_sem,
			                    shm_ptr,
			                    &shm_fd);
			return 0;
		}



		/*TODO handle the server queue */
	}
	// exit of main while loop

	/* WARNING Control should never reach here */
	fprintf(stderr, "Error: CONTROL escaped normal loop\n");
	return 1;
}
