#include <time.h>
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

/* Signal handlers for handling SIGTERM signal from coordinator process */
void sigterm_handler(int sig_num);
void sigint_handler(int sig_num);

int main(int argc, char const *argv[]) {
	/* cmd args validation
	    ./server -m shmid */
	if ( argc != 3 || strcmp(argv[1], "-m") != 0 || isdigit_all(argv[2], strlen(argv[2]))) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./server -m shmid\n");
		return 1;
	}
	/* Setting the SIGINT (Ctrl-C) signal handler to sigint_handler */
	signal(SIGINT, sigint_handler);
	/* Setting the SIGTERM signal handler to sigterm_handler */
	signal(SIGTERM, sigterm_handler);
	srand(time(NULL));     // seed the random number generator

	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	int num_menu_items_temp = num_menu_items(menu_file);
	struct Item menu_items[num_menu_items_temp];
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

	/* if shutting down has already been initiated by the coordinator */
	if (shm_ptr->initiate_shutdown == 1) {
		/* Clean up normally */
		server_exit_cleanup(server_sem, server_cli_q_sem, deq_s_block_sem, shm_write_sem,
		                    shutdown_sem, shm_ptr, &shm_fd);
		return 0;
	}
	/* Check if more than one servers are being tried to be initiated */
	/* add Server pid to the server pid var in shared mem */
	if (shm_ptr->server_pid == NO_SERVER_TEMP_PID) {
		////////* Acquire semaphore lock first before writing in shared memory */                                                                 //
		TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
		shm_ptr->server_pid = getpid();
		/* release semaphore write lock after writing to shared memory */
		TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
		////////////////////////////////////////////////////////////////////////

	}
	else {
		printf("Only one server is allowed in the restaurant\n");
		/* Clean up normally */
		server_exit_cleanup(server_sem, server_cli_q_sem, deq_s_block_sem, shm_write_sem,
		                    shutdown_sem, shm_ptr, &shm_fd);
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in Cashier begins *//////////////////////
	////////////////////////////////////////////////////////////////////////////

	while (1) {
		/* if shutting down has been initiated by the coordinator */
		if (shm_ptr->initiate_shutdown == 1) {
			/* Clean up normally */
			server_exit_cleanup(server_sem, server_cli_q_sem, deq_s_block_sem, shm_write_sem,
			                    shutdown_sem, shm_ptr, &shm_fd);
			return 0;
		}
		/* Server will wait for a client to reach out for order */              // wait ( Ses )
		TRY_AND_CATCH_INT(sem_wait(server_sem), "sem_wait()");

		/* write time spent with client on the shared memory segment */
		/* Check the client_record_array for pid of current client process
		    to write the allocated serving time to the
		    time_with_server attribute */
		int temp_server_cli_serving_time = 0;
		for (int i = 0; i < shm_ptr->cur_client_record_size; i++) {
			if ((shm_ptr->client_record_array[i]).client_pid == (shm_ptr->client_server_queue[shm_ptr->front_server_Q]).client_pid ) {
				int menu_item_id = ((shm_ptr->client_server_queue[shm_ptr->front_server_Q]).menu_item_id )-1; // sub 1 to match item order in file
				int menu_max_time = 1;
				int menu_min_time = 1;

				/* loop through the menu_items struct to find the ordered item */
				for (int j = 0; j < num_menu_items_temp; j++) {
					if (menu_item_id == menu_items[j].menu_item_id) {
						menu_max_time = menu_items[j].menu_max_time;
						menu_min_time = menu_items[j].menu_min_time;
						break;
					}
				}
				/* Choose a serving time between min and max serving time of food ordered */
				temp_server_cli_serving_time = rand() % (((menu_max_time-menu_min_time)+1) + menu_min_time);
				(shm_ptr->client_record_array[i]).time_with_server = temp_server_cli_serving_time;
				printf("Currently serving Client %li, %s which will be ready in %i s.\n",
					(long)(shm_ptr->client_record_array[i]).client_pid, menu_items[menu_item_id].menu_desc, temp_server_cli_serving_time);
				break;
			}
		}
		/* Server will allow client to leave after the serving has been done
		    and dequeue the client from the client_server_queue */
		TRY_AND_CATCH_INT(sem_post(deq_s_block_sem), "sem_post()");				// signal ( DeqS )
	}
	// exit of main while loop

	/* WARNING Control should never reach here */
	fprintf(stderr, "Error: CONTROL escaped normal loop\n");
	return 1;
}

/* Signal Handler for SIGINT */
void sigint_handler(int sig_num) {
	/* Reset handler to catch SIGINT next time */
	signal(SIGINT, sigint_handler);
	printf("Server is terminating after getting a Ctrl+C SIGINT signal\n");
	fflush(stdout);
	exit(0);
}

/* Signal Handler for SIGTERM */
void sigterm_handler(int sig_num) {
	/* Reset handler to catch SIGTERM next time */
	signal(SIGTERM, sigint_handler);
	printf("Server with %li has received orders to shutdown from coordinator.\n", (long)getpid() );
	fflush(stdout);
	exit(0);
}
