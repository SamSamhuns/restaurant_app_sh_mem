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

void sigterm_handler(int sig_num);
void sigint_handler(int sig_num);

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

	/* loading the menu items from the txt file into a menu_items struct*/
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);

	/* Setting the SIGINT (Ctrl-C) signal handler to sigint_handler */
	signal(SIGINT, sigint_handler);
	/* Setting the SIGTERM signal handler to sigterm_handler */
	signal(SIGTERM, sigterm_handler);

	/* INITIALIZE THE SHARED MEMORY STRUCTURE */

	/* name of the shared memory object / SHMID */
	fprintf(stdout, "Shared Memory Restaurant SHMID is %s\n", SHMID);

	/* shared memory file descriptor */
	int shm_fd;
	struct Shared_memory_struct *shared_mem_ptr;

	/* named semaphores initialization */
	sem_t *cashierS = sem_open(CASHIER_SEM,
	                           O_CREAT | O_EXCL, 0666, 0); /* Init cashierS semaphore to 0 */
	sem_t *clientQS = sem_open(CLIENTQ_SEM,
	                           O_CREAT | O_EXCL, 0666, 0); /* Init clientQS sempaphore to 0 */
	sem_t *shared_mem_write_sem = sem_open(SHARED_MEM_WR_LOCK_SEM,
	                                       O_CREAT | O_EXCL, 0666, 1); /* Init shared mem write sempaphore to 1 */
	TRY_AND_CATCH_SEM(cashierS, "sem_open()");
	TRY_AND_CATCH_SEM(clientQS, "sem_open()");
	TRY_AND_CATCH_SEM(shared_mem_write_sem, "sem_open()");

	/* create the shared memory object O_EXCL flag gives error if a shared memory
	    with the given name already exists */
	shm_fd = shm_open(SHMID, O_CREAT | O_RDWR | O_EXCL, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* configure the size of the shared memory object */
	if (ftruncate(shm_fd, sizeof(Shared_memory_struct)) == -1 )
	{
		perror("ftruncate");
		exit(1);
	}

	/* memory map the shared memory object */
	if ((shared_mem_ptr = mmap(0, sizeof(Shared_memory_struct),
	                           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	};

	printf("DEBUG size of shared memory is %zi\n",sizeof(Shared_memory_struct) );
	/* write to the shared memory object */

	/* Acquire semaphore lock first before writing */
	if (sem_wait(shared_mem_write_sem) == -1) {
		perror("sem_wait()");
		exit(1);
	}
	shared_mem_ptr->front_client_Q = -1;
	shared_mem_ptr->rear_client_Q = -1;
	shared_mem_ptr->size_client_Q = -1;
	shared_mem_ptr->front_server_Q = -1;
	shared_mem_ptr->rear_server_Q = -1;
	shared_mem_ptr->size_server_Q = -1;
	shared_mem_ptr->client_record_cur_size = 0;
	shared_mem_ptr->MaxCashiers = MaxCashiers;
	shared_mem_ptr->MaxPeople = MaxPeople;
	shared_mem_ptr->server_pid = -1;
	shared_mem_ptr->totalCashierNum = 0;
	shared_mem_ptr->totalClientNum = 0;
	shared_mem_ptr->totalClientOverall = 0;

	/* release semaphore write lock after writing to shared memory */
	if (sem_post(shared_mem_write_sem) == -1) {
		perror("sem_post()");
		exit(1);
	}

	int cc;
	printf("REMOVE LATER Scanning an int for TEMP SYNCHRONIZATION:\n");
	scanf("%d",&cc);

	sleep(5); // To give enough time for clients to arrive in the beginning
	/* if there are no clients in the restaurant then sleep for a while
	    and check if still no clients then close restaurant and
	    send kill signals to all cashiers and the server */
	if ( shared_mem_ptr->totalClientNum == 0 ) {
		sleep(MaxTimeWait);
		if ( shared_mem_ptr->totalClientNum == 0 ) {
			/* code */
		}
	}




	/*EXIT CLEAN UP*/
	/* close the named semaphores */
	sem_close(cashierS);
	sem_close(clientQS);
	/* remove the named semaphores
	    IMPORTANT only coordinator should call this
	    at the end */
	sem_unlink(CASHIER_SEM);
	sem_unlink(CLIENTQ_SEM);

	munmap(shared_mem_ptr, sizeof(Shared_memory_struct));  /* unmap the shared mem object */
	close(shm_fd);  /* close the file descrp from shm_open */
	/* remove the shared mem object
	    IMPORTANT only coordinator should call this
	    at the end */
	shm_unlink(SHMID);


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
			*MaxTimeWait = strtol(argv[i+1], NULL, 10);
		}
	}
	if (maxC_found == 1 && maxP_found == 1 && maxT_found == 1) {
		return 0;
	}
	return 1; // if the correct args were not found
}

/* Signal Handler for SIGINT */
void sigint_handler(int sig_num) {
	/* Reset handler to catch SIGINT next time */
	signal(SIGINT, sigint_handler);
	printf("\nGraceful termination and cleaning shm_mem with Ctrl+C\n");
	fflush(stdout);

	/*EXIT CLEAN UP*/
	coordinator_exit_cleanup ();
	exit(0);
}

/* Signal Handler for SIGTERM */
void sigterm_handler(int sig_num) {
	/* Reset handler to catch SIGTERM next time */
	signal(SIGTERM, sigint_handler);
	printf("\nGraceful termination and cleaning shm_mem after recv sigterm\n");
	fflush(stdout);

	/*EXIT CLEAN UP*/
	coordinator_exit_cleanup ();
	exit(0);
}
