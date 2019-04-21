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

/* cmd args validation
    ./cashier -s serviceTime -b breakTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *serviceTime, char *shmid, long *breakTime);

int main(int argc, char const *argv[]) {
	long serviceTime, breakTime;
	char shmid[MAX_SHMID_LEN];
	srand(time(NULL)); // seed the random number generator
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
	sem_t *cashier_sem = sem_open(CASHIER_SEM, 0); /* open existing server_sem semaphore */
	sem_t *cashier_cli_q_sem = sem_open(CASHIER_CLI_Q_SEM, 0); /* open existing server_cli_q_sem semaphore */
	sem_t *deq_c_block_sem = sem_open(DEQ_C_BLOCK_SEM, 0); /* open existing deq_s_block_sem semaphore */
	sem_t *shm_write_sem = sem_open(SHM_WRITE_SEM, 0); /* open existing server_sem semaphore */
	sem_t *shutdown_sem = sem_open(SHUTDOWN_SEM, 0); /* open existing server_sem semaphore */
	TRY_AND_CATCH_SEM(cashier_sem, "sem_open()");
	TRY_AND_CATCH_SEM(cashier_cli_q_sem, "sem_open()");
	TRY_AND_CATCH_SEM(deq_c_block_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shm_write_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shutdown_sem, "sem_open()");

	/* shared memory file descriptor */
	int shm_fd;
	struct Shared_memory_struct *shm_ptr;

	/* open the shared memory object */
	shm_fd = shm_open(shmid, O_RDWR, 0666);
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
		cashier_exit_cleanup(cashier_sem, cashier_cli_q_sem, deq_c_block_sem,
		                     shm_write_sem, shutdown_sem, shm_ptr, &shm_fd);
		return 0;
	}
	/* if maximum number of cashiers have been reached */
	if ( (shm_ptr->cur_cashier_num)+1 > shm_ptr->MaxCashiers ) {
		printf("Cashier will not be joining restaurant as max num of cashiers exceeded\n");
		/* Clean up normally */
		cashier_exit_cleanup(cashier_sem, cashier_cli_q_sem, deq_c_block_sem,
		                     shm_write_sem, shutdown_sem, shm_ptr, &shm_fd);
		return 0;
	}
	/* join the cashier list by adding cashier pid to cashier_pid_queue */
	else {
		//////* Acquire semaphore lock first before writing in shared memory *//////
		if (sem_wait(shm_write_sem) == -1) {
			perror("sem_wait()");
			exit(1);
		}
		int cashier_pos_temp = shm_ptr->cur_cashier_num;
		shm_ptr->cashier_pid_array[cashier_pos_temp] = getpid();
		shm_ptr->cur_cashier_num += 1;

		/* release semaphore write lock after writing to shared memory */
		if (sem_post(shm_write_sem) == -1) {
			perror("sem_post()");
			exit(1);
		}
		////////////////////////////////////////////////////////////////////////////
	}

	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in Cashier begins *//////////////////////
	////////////////////////////////////////////////////////////////////////////

	while (1) {
		/* if shutting down has been initiated by the coordinator */
		if (shm_ptr->initiate_shutdown == 1) {
			/* Clean up normally */
			cashier_exit_cleanup(cashier_sem, cashier_cli_q_sem, deq_c_block_sem,
			                     shm_write_sem, shutdown_sem, shm_ptr, &shm_fd);
			return 0;
		}
		/* Cashier locks the cashier_sem semaphore before proceeding to make sure
		   not two cashiers grab the same client */
		////////////////////////////////////////////////////////////////////////////
		if (sem_wait(cashier_sem) == -1) {                                         // wait(CaS)
			perror("sem_wait()");
			exit(1);
		}
		/* If there are no clients currently waiting to be processed in the cashier client queue
		   take a break in the interval [1...breakTime] */
		if (shm_ptr->size_client_Q <= 0) {
			/* Cashier releases cashier_sem lock */                                         // Signal (CaS)
			if (sem_post(cashier_sem) == -1) {
				perror("sem_post()");
				exit(1);
			}
			int temp_sleep_time = rand() % (((int)breakTime)+1);
			if (temp_sleep_time == 0) { // if the rand created a perfectly divisible num
				temp_sleep_time = 1;
			}
			printf("No clients currently in cashier queue. Cashier taking a break for %i s\n",
			       temp_sleep_time);
			sleep(temp_sleep_time);
		}

		/* if there are clients queuing up in the client cashier queue */
		else {
			/* Cashier locks the cashier_cli_q_sem  semaphore */                                  // wait (CiQS)
			if (sem_wait(cashier_cli_q_sem ) == -1) {
				perror("sem_wait()");
				exit(1);
			}

			/* Write client information to the shared memory in client_record_array */
			//////* Acquire semaphore lock first before writing in shared memory *//////
			if (sem_wait(shm_write_sem) == -1) {
				perror("sem_wait()");
				exit(1);
			}

			int cli_record_index = shm_ptr->cur_client_record_size;
			/* decrement by 1 as the menu items are zero indexed in menu_items struct */
			int menu_item_id = ((shm_ptr->client_cashier_queue[shm_ptr->front_client_Q]).menu_item_id)-1;
			/* save client pid */
			(shm_ptr->client_record_array[cli_record_index]).client_pid = \
				(shm_ptr->client_cashier_queue[shm_ptr->front_client_Q]).client_pid;
			/* save client menu item id */
			(shm_ptr->client_record_array[cli_record_index]).menu_item_id = menu_item_id;
			/* save desc of item ordered */
			strcpy( (shm_ptr->client_record_array[cli_record_index]).menu_desc, menu_items[menu_item_id].menu_desc );
			/* save price of item ordered */
			(shm_ptr->client_record_array[cli_record_index]).menu_price = menu_items[menu_item_id].menu_price;

			/* Generate a random time to serve the client in the interval [1...serviceTime] */
			int temp_sleep_time = rand() % (((int)serviceTime)+1);
			if (temp_sleep_time == 0) {     // if the rand created a perfectly divisible num
				temp_sleep_time = 1;
			}
			printf("Server %li currently serving Client %li with serve time %i \n",
			       (long) getpid(),
			       (long) (shm_ptr->client_record_array[cli_record_index]).client_pid,
			       temp_sleep_time);

			(shm_ptr->client_record_array[cli_record_index]).time_with_cashier = temp_sleep_time;

			/* after this is set, the pending client should run immediately */
			shm_ptr->cur_client_record_size += 1;

			/* release semaphore write lock after writing to shared memory */
			if (sem_post(shm_write_sem) == -1) {
				perror("sem_post()");
				exit(1);
			}
			////////////////////////////////////////////////////////////////////////////

			/*remove the client from the queue after writing its information to the
			    client client_record_array */
			dequeue_client_cashier_q(shm_ptr, shm_write_sem);

			/* Cashier releases the cashier_sem lock */                             // Signal (CaS)
			if (sem_post(cashier_sem) == -1) {
				perror("sem_post()");
				exit(1);
			}
			/* Cashier releases the cashier_cli_q_sem  semaphore */                        // Signal (CiQS)
			if (sem_post(cashier_cli_q_sem ) == -1) {
				perror("sem_wait()");
				exit(1);
			}
			/* Only sleep after lock on the cashier_sem has been lifted */
			sleep(temp_sleep_time); /* cashier serves client */
		}
		////////////////////////////////////////////////////////////////////////////
	}
	// exit of main while loop
	/* WARNING Control should never reach here */
	fprintf(stderr, "Error: CONTROL escaped normal loop\n");
	return 1;
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
