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
    ./client -i itemId -e eatTime -m shmid	*/
int cmd_validate(int argc, char const *argv[], long *itemId, char *shmid, long *eatTime);

int main(int argc, char const *argv[]){
	long itemId, eatTime;
	char shmid[MAX_SHMID_LEN];
	srand(time(NULL)); // seed the random number generator

	/* cmd args validation
	    ./client -i itemId -e eatTime -m shmid	*/
	if (cmd_validate(argc, argv, &itemId, shmid, &eatTime) == 1) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./client -i itemId -e eatTime -m shmid\n");
		return 1;
	}

	/* loading the menu items from the txt file into a menu_items struct*/
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	int num_menu_items_temp = num_menu_items(menu_file);
	/* Check if client ordered outside the existing menu items */
	if (itemId > num_menu_items_temp) {
		fprintf(stderr, "Item id(%li) exceeds the number of available items(%i) in menu.\n", itemId, num_menu_items_temp);
		return 0;
	}
	struct Item menu_items[num_menu_items_temp];
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

	////////////////////////////////////////////////////////////////////////////
	////////////////////* Main section in Client begins *///////////////////////
	////////////////////////////////////////////////////////////////////////////
	int client_not_enter_restaurant = 0;

	/* if restaurant is not properly staffed */
	if (shared_mem_ptr->cur_cashier_num == 0 || shared_mem_ptr->server_pid == NO_SERVER_TEMP_PID) {
		printf("Client will leave because restaurant is not properly staffed\n");
		client_not_enter_restaurant = 1;
	}
	/* If the current num of clients being served in the restaurant has reached MaxPeople limit */
	else if (shared_mem_ptr->cur_client_num+1 > shared_mem_ptr->MaxPeople) {
		printf("Client will not be joining restaurant queue as it is full right now\n");
		client_not_enter_restaurant = 1;
	}
	/* If the overall number of clients has exceeded the MAX_REST_QUEUE_CAP restaurant global capacity */
	else if (shared_mem_ptr->overall_client_num+1 > MAX_REST_QUEUE_CAP) {
		printf("Client will leave because restaurant has reached its capacity and will not take any more clients today\n");
		client_not_enter_restaurant = 1;
	}
	/* if shutting down has been initiated by the coordinator */
	else if (shared_mem_ptr->initiate_shutdown == 1) {
		printf("Client will leave because restaurant is going to shut down now\n");
		client_not_enter_restaurant = 1;
	}


	if (client_not_enter_restaurant) {
		/* Client will not enter restaurant so clean up normally and exit */
		all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
		return 0;
	}

	/* If control reaches here Now we can increment cur_client_num, overall_client_num
			and add clients first to the client_cashier_queue */
	////////* Acquire semaphore lock first before writing in shared memory *//////
	if (sem_wait(shared_mem_write_sem) == -1) {
		perror("sem_wait()");
		exit(1);
	}
	shared_mem_ptr->cur_client_num += 1;
	shared_mem_ptr->overall_client_num += 1;

	/* release semaphore write lock after writing to shared memory */
	if (sem_post(shared_mem_write_sem) == -1) {
		perror("sem_post()");
		exit(1);
	}
	//////////////////////////////////////////////////////////////////////////////

	////////* Client locks the clientQS semaphore by calling wait on it */////////
	//////////////////////////////////////////////////////////////////////////////
	if (sem_wait(clientQS) == -1) {
		perror("sem_wait()");
		exit(1);
	}

	/* Client joins the client_cashier_queue */
	enqueue_client_cashier_q(shared_mem_ptr, itemId, shared_mem_write_sem);

	/* signal sempahore after adding itself to the client_cashier_queue */////////
	if (sem_post(clientQS) == -1) {
		perror("sem_post()");
		exit(1);
	}
	//////////////////////////////////////////////////////////////////////////////

	/* IMPORTANT have to make the client stop here at this point before proceeding */
	/* Client waits till cashier has set a serving time for both */
	int temp_breaker = 1;
	while (temp_breaker) {
		for (int i = shared_mem_ptr->front_client_Q; i < shared_mem_ptr->cur_client_record_size; i++) {
			if ( (shared_mem_ptr->client_record_array[i]).client_pid == getpid() ) {
				printf("Being served by a cashier right now\n");
				temp_breaker = 0;
				break;
			}
		}
	}
	sleep((shared_mem_ptr->client_record_array[shared_mem_ptr->cur_client_record_size]).time_with_cashier); /* client is being serve by the cashier */

	/*TODO, get order with server*/





	/* Eat at the restaurant */
	int temp_sleep_time = rand() % (((int)eatTime)+1);
	if (temp_sleep_time == 0) { // if the rand created a perfectly divisible num
		temp_sleep_time = 1;
	}
	printf("Client %li is eating for %i s before leaving\n", (long)getpid(), temp_sleep_time );
	sleep(temp_sleep_time);

	/* Decrement the cur_client_num counter before leaving */
	////////* Acquire semaphore lock first before writing in shared memory *//////
	if (sem_wait(shared_mem_write_sem) == -1) {
		perror("sem_wait()");
		exit(1);
	}
	shared_mem_ptr->cur_client_num -= 1;

	/* release semaphore write lock after writing to shared memory */
	if (sem_post(shared_mem_write_sem) == -1) {
		perror("sem_post()");
		exit(1);
	}
	//////////////////////////////////////////////////////////////////////////////

	printf("Client with ID %i has successfully ordered, dined and left the restaurant\n", getpid());
	/* Clean up normally */
	all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
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
