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

/* functions for handling signals only for coordinator
    WARNING using SIGINT/ Ctrl ^ C will not kill other Cashier,
    Server and Client processes
    To kill all other processes coordinator must have a normal shutdown */
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

	/* Setting the SIGINT (Ctrl-C) signal handler to sigint_handler */
	signal(SIGINT, sigint_handler);
	/* Setting the SIGTERM signal handler to sigterm_handler */
	signal(SIGTERM, sigterm_handler);

	/////////////////* INITIALIZE THE SHARED MEMORY STRUCTURE *///////////////////
	/* name of the shared memory object / SHMID */
	fprintf(stdout, "Shared Memory Restaurant SHMID is %s\n", SHMID);
	int shm_fd; /* shared memory file descriptor */
	struct Shared_memory_struct *shared_mem_ptr;

	/* named semaphores initialization */
	sem_t *cashierS = sem_open(CASHIER_SEM,
	                           O_CREAT | O_EXCL, 0666, 1); /* Init cashierS semaphore to 1 */
	sem_t *clientQS = sem_open(CLIENTQ_SEM,
	                           O_CREAT | O_EXCL, 0666, 1); /* Init clientQS sempaphore to 1 */
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
	if (ftruncate(shm_fd, sizeof(Shared_memory_struct)) == -1 ) {
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

	///////////////* Acquire semaphore lock first before writing *////////////////
	//////////////////////////////////////////////////////////////////////////////
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
	shared_mem_ptr->cur_client_record_size = 0;
	shared_mem_ptr->MaxCashiers = MaxCashiers; // should not change later
	shared_mem_ptr->MaxPeople = MaxPeople;  // should not change later
	shared_mem_ptr->initiate_shutdown = 0; // shutdown not initiated in the beginning
	shared_mem_ptr->server_pid = NO_SERVER_TEMP_PID; // place holder server_pid
	shared_mem_ptr->cur_cashier_num = 0;
	shared_mem_ptr->cur_client_num = 0;
	shared_mem_ptr->overall_client_num = 0;

	/* release semaphore write lock after writing to shared memory */
	if (sem_post(shared_mem_write_sem) == -1) {
		perror("sem_post()");
		exit(1);
	}
	//////////////////////////////////////////////////////////////////////////////

	int cc;
	printf("REMOVE LATER Scanning an int for TEMP SYNCHRONIZATION:\n");
	scanf("%d",&cc);

	sleep(2); // To give enough time for clients to arrive in the beginning

	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in coordinator begins *//////////////////
	////////////////////////////////////////////////////////////////////////////
	while (1) {
		/* if there are no clients in the restaurant then sleep for a while
		    and check if still no clients then close restaurant and
		    send kill signals to all cashiers and the server */
		if ( shared_mem_ptr->cur_client_num == 0 ) {
			sleep(MaxTimeWait);
			/* if still no clients present then initiate restaurant shutdown */
			if ( shared_mem_ptr->cur_client_num == 0 ) {

				////* Acquire semaphore lock first before writing in shared memory *////
				////////////////////////////////////////////////////////////////////////
				if (sem_wait(shared_mem_write_sem) == -1) {														//
					perror("sem_wait()");																								//
					exit(1);																														//
				}																																			//
				// initiate shutdown for all processes and take no more clients				//																													//
				shared_mem_ptr->initiate_shutdown = 1;																//																		//
				/* release semaphore write lock after writing to shared memory */			//
				if (sem_post(shared_mem_write_sem) == -1) {														//
					perror("sem_post()");																								//
					exit(1);																														//
				}																																			//
				////////////////////////////////////////////////////////////////////////

				///////////////////////*  GENERATE STATISTICS  *////////////////////////
				/* create a new stats folder and write to a statistics file*/
				struct stat st = {0};

				/* Create a mew stats directory if a current one doesn't already exist */
				if (stat("stats", &st) == -1) {
				    mkdir("stats", 0700);
				}

				/* Write a statistic_CURRENT_UNIX_TIME.txt file for statistics each time */
				char f_stat_name[0x100];
				sprintf(f_stat_name,"stats/statistics_%lu.txt", (unsigned long)time(NULL));
				FILE *f_stats = fopen(f_stat_name, "w");
				fprintf(f_stats, "Client_pid, item_ordered, money_spent($), eat_time, time_with_cashier, time_with_server, total_time_spent\n");

				int cur_client_record_size = shared_mem_ptr->cur_client_record_size;
				for (int i = 0; i < cur_client_record_size; i++) {
						long client_pid = (long) ((shared_mem_ptr->client_record_array[i]).client_pid);
						char menu_desc[MAX_ITEM_DESC_LEN];
						strcpy(menu_desc, ((shared_mem_ptr->client_record_array[i]).menu_desc));
						int menu_price = ((shared_mem_ptr->client_record_array[i]).menu_price);
						int eat_time = ((shared_mem_ptr->client_record_array[i]).eat_time);
						int time_with_cashier = ((shared_mem_ptr->client_record_array[i]).time_with_cashier);
						int time_with_server = ((shared_mem_ptr->client_record_array[i]).time_with_server);
						int total_time_spent = eat_time + time_with_cashier + time_with_server;
						printf("Client with ID %li spent %i s with the cashier, %i s waiting for food and %i eating. \
									In total, they spent %i s in the restaurant eating %s and spent a total of %i dollars.\n",
							client_pid, time_with_cashier, time_with_server, eat_time, total_time_spent, menu_desc, menu_price);

						fprintf(f_stats, "%li, %s, %i, %i, %i, %i, %i\n",
										client_pid, menu_desc, menu_price, eat_time, time_with_cashier, time_with_server, total_time_spent);
				}
				fclose(f_stats);
				////////////////////////* NORMAL EXIT CLEAN UP*/////////////////////////
				/////////////  FORCEFUL SHUTDOWN FROM COORDINATOR  /////////////
				/* kill the server process if open */
				// if (kill( shared_mem_ptr->server_pid, SIGTERM) == -1 ) {
				// 	fprintf(stderr, "Server process does not exist\n");
				// }
				// else {
				// 	fprintf(stdout, "Shutting Server with pid %li\n",
				// 	        (long)shared_mem_ptr->server_pid);
				// }
				//
				// /* kill all cashier processes if open */
				// int cur_cashier_num = shared_mem_ptr->cur_cashier_num;
				// for (int i = 0; i < cur_cashier_num; i++) {
				// 	pid_t cashier_pid = shared_mem_ptr->cashier_pid_array[i];
				// 	printf("Shutting cashier with pid %li\n", (long)cashier_pid );
				// 	if (kill( cashier_pid, SIGTERM) == -1 ) {
				// 		fprintf(stderr, "Cashier with pid %li does not exist\n",
				// 		        (long)cashier_pid);
				// 	};
				// }
				////////////////////////////////////////////////////////////////////////

				// close all sems and shm
				all_exit_cleanup(clientQS, cashierS, shared_mem_write_sem, shared_mem_ptr, &shm_fd);
				// unlink all sems and shm
				coordinator_only_exit_cleanup();
				fprintf(stdout, "Shutting down restaurant normally\n");
				return 0; // exit normally
			}
		}
	}
	/* WARNING Control should never reach here */
	fprintf(stderr, "Error: CONTROL escaped normal loop\n");
	return 1;
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
	/* additional check if MaxPeople and MaxCashiers are valid */
	if (*MaxCashiers > MAX_CASHIER_CAP) {
		fprintf(stderr,
		        "Max number of cashiers (%li) cannot exceed global Maximum (%d)\n", *MaxCashiers, MAX_CASHIER_CAP);
		fprintf(stderr, "Change Global max in common.h\n");
		exit(1);
	}
	if (*MaxPeople > MAX_REST_QUEUE_CAP) {
		fprintf(stderr,
		        "Max number of people servicable (%li) cannot exceed global Maximum (%d)\n", *MaxPeople, MAX_REST_QUEUE_CAP);
		fprintf(stderr, "Change Global max in common.h\n");
		exit(1);
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
	coordinator_only_exit_cleanup ();
	exit(0);
}

/* Signal Handler for SIGTERM */
void sigterm_handler(int sig_num) {
	/* Reset handler to catch SIGTERM next time */
	signal(SIGTERM, sigint_handler);
	printf("\nGraceful termination and cleaning shm_mem after recv sigterm\n");
	fflush(stdout);

	/*EXIT CLEAN UP*/
	coordinator_only_exit_cleanup ();
	exit(0);
}
