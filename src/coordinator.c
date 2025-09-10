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
	struct Shared_memory_struct *shm_ptr;

	/* named semaphores initialization */
	sem_t *cashier_sem = sem_open(CASHIER_SEM, O_CREAT | O_EXCL, 0666, 1); /* Init cashier_sem semaphore to 1 */
	sem_t *cashier_cli_q_sem = sem_open(CASHIER_CLI_Q_SEM, O_CREAT | O_EXCL, 0666, 1); /* Init cashier_cli_q_sem semaphore to 1 */
	sem_t *deq_c_block_sem = sem_open(DEQ_C_BLOCK_SEM, O_CREAT | O_EXCL, 0666, 0); /* Init deq_c_block_sem semaphore to 0 */
	sem_t *server_sem = sem_open(SERVER_SEM, O_CREAT | O_EXCL, 0666, 0); /* Init server_sem semaphore to 0 */
	sem_t *server_cli_q_sem = sem_open(SERVER_CLI_Q_SEM, O_CREAT | O_EXCL, 0666, 1); /* Init server_cli_q_sem semaphore to 1 */
	sem_t *deq_s_block_sem = sem_open(DEQ_S_BLOCK_SEM, O_CREAT | O_EXCL, 0666, 0);	 /* Init deq_s_block_sem semaphore to 0 */
	sem_t *shm_write_sem = sem_open(SHM_WRITE_SEM, O_CREAT | O_EXCL, 0666, 1);		 /* Init shm_write_sem semaphore to 1 */
	sem_t *shutdown_sem = sem_open(SHUTDOWN_SEM, O_CREAT | O_EXCL, 0666, 0);		 /* Init shutdown_sem semaphore to 0 */
	TRY_AND_CATCH_SEM(cashier_sem, "sem_open()");
	TRY_AND_CATCH_SEM(cashier_cli_q_sem, "sem_open()");
	TRY_AND_CATCH_SEM(deq_c_block_sem, "sem_open()");
	TRY_AND_CATCH_SEM(server_sem, "sem_open()");
	TRY_AND_CATCH_SEM(server_cli_q_sem, "sem_open()");
	TRY_AND_CATCH_SEM(deq_s_block_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shm_write_sem, "sem_open()");
	TRY_AND_CATCH_SEM(shutdown_sem, "sem_open()");

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
	if ((shm_ptr = mmap(0, sizeof(Shared_memory_struct),
	                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	};

	/* write to the shared memory object */

	///////////////* Acquire semaphore lock first before writing *//////////////
	////////////////////////////////////////////////////////////////////////////
	TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
	shm_ptr->front_client_Q = -1;
	shm_ptr->rear_client_Q = -1;
	shm_ptr->size_client_Q = -1;
	shm_ptr->front_server_Q = -1;
	shm_ptr->rear_server_Q = -1;
	shm_ptr->size_server_Q = -1;
	shm_ptr->cur_client_record_size = 0;
	shm_ptr->MaxCashiers = MaxCashiers; // should not change later
	shm_ptr->MaxPeople = MaxPeople;  // should not change later
	shm_ptr->initiate_shutdown = 0; // shutdown not initiated in the beginning
	shm_ptr->server_pid = NO_SERVER_TEMP_PID; // place holder server_pid
	shm_ptr->cur_cashier_num = 0;
	shm_ptr->cur_client_num = 0;
	shm_ptr->overall_client_num = 0;
	/* release semaphore write lock after writing to shared memory */
	TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
	////////////////////////////////////////////////////////////////////////////

	sleep(12); // To give enough time for clients to arrive in the beginning
	// Check if any client processes are in the Restaurant, if no signal shutdown semaphore
	if (shm_ptr->cur_client_num == 0) {
		TRY_AND_CATCH_INT(sem_post(shutdown_sem), "sem_post()");
	}

	////////////////////////////////////////////////////////////////////////////
	/////////////////* Main while loop in coordinator begins *//////////////////
	////////////////////////////////////////////////////////////////////////////
	while (1) {
		/* if there are no clients in the restaurant then sleep for a while
		    and check if still no clients then close restaurant and
		    send kill signals to all cashiers and the server */
		TRY_AND_CATCH_INT(sem_wait(shutdown_sem), "sem_wait()");
		if ( shm_ptr->cur_client_num == 0 ) {
			sleep(MaxTimeWait);
			/* if still no clients present then initiate restaurant shutdown */
			if ( shm_ptr->cur_client_num == 0 ) {

				////* Acquire semaphore lock first before writing in shared memory *////
				////////////////////////////////////////////////////////////////////////
				TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");             //
				// initiate shutdown for all processes and take no more clients		  //
				shm_ptr->initiate_shutdown = 1;                                       //
				/* release semaphore write lock after writing to shared memory */     //
				TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");             //                                                             //
				////////////////////////////////////////////////////////////////////////
				/* kill the server process if open */
				if (kill( shm_ptr->server_pid, SIGTERM) == -1 ) {
					fprintf(stderr, "Server process does not exist\n");
				}
				else {
					fprintf(stdout, "Shutting Server with pid %li\n",
					        (long)shm_ptr->server_pid);
				}

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

				int cur_client_record_size = shm_ptr->cur_client_record_size;
				float total_revenue_generated = 0;
				int total_waiting_time_for_all_clients = 0;
				/* A struct array to hold count of ordered items to evaluate the five most popular items
				    The size is cur_client_record_size+1 as there need not be more types of food that what the
				    clients ordered in our final statistic */
				struct Menu_Count_Item menu_item_counts[(cur_client_record_size)+1];
				int menu_item_counts_index = 0; // tracks how many unique items have been added to menu_item_counts array

				for (int i = 0; i < cur_client_record_size; i++) {
					long client_pid = (long) ((shm_ptr->client_record_array[i]).client_pid);
					char menu_desc[MAX_ITEM_DESC_LEN];
					strcpy(menu_desc, ((shm_ptr->client_record_array[i]).menu_desc));
					float menu_price = ((shm_ptr->client_record_array[i]).menu_price);
					int eat_time = ((shm_ptr->client_record_array[i]).eat_time);
					int time_with_cashier = ((shm_ptr->client_record_array[i]).time_with_cashier);
					int time_with_server = ((shm_ptr->client_record_array[i]).time_with_server);
					int total_time_spent = eat_time + time_with_cashier + time_with_server;
					total_waiting_time_for_all_clients += total_time_spent;
					total_revenue_generated += menu_price;
					printf("Client with ID %li spent %is with the cashier, %is waiting for food and %is eating %s. In total, they spent %is in the restaurant and spent a total of $ %.2f.\n",
					       client_pid, time_with_cashier, time_with_server, eat_time, menu_desc, total_time_spent, menu_price);

					fprintf(f_stats, "%li, %s, %.2f, %i, %i, %i, %i\n",
					        client_pid, menu_desc, menu_price, eat_time, time_with_cashier, time_with_server, total_time_spent);

					int item_found = 0;
					/* Check if the item exists in our record array */
					for (int j = 0; j < (cur_client_record_size)+1; j++) {
						if (strcmp(menu_desc, menu_item_counts[j].menu_desc) == 0) {
							item_found = 1;
							menu_item_counts[j].menu_total_price += menu_price;
							menu_item_counts[j].menu_item_total_count += 1;
							break;
						}
					}
					/* if item does not already exist in the count struct array */
					if (item_found == 0) {
						strcpy(menu_item_counts[menu_item_counts_index].menu_desc, menu_desc);
						menu_item_counts[menu_item_counts_index].menu_total_price = menu_price;
						menu_item_counts[menu_item_counts_index].menu_item_total_count = 1;
						menu_item_counts[menu_item_counts_index].chosen_for_top_five_already = 0;
						menu_item_counts_index += 1;
					}
				}

				/*Only generate the following statistics if at least one client has arrived */
				if (cur_client_record_size > 0 ) {
					/*Average waiting time for all clients after entering diner and leaving it */
					printf("\nAverage waiting time for all clients after entering the diner to leaving it is %.2fs\n",(float)total_waiting_time_for_all_clients/(float)(shm_ptr->overall_client_num ));
					fprintf(f_stats,"Average waiting time for all clients after entering diner and leaving it is %.2fs\n",(float)total_waiting_time_for_all_clients/(float)(shm_ptr->overall_client_num));
					/*Printing total number of clients visited and total revenue for the day*/
					printf("Total number of Clients who visited our restaurant is %i.\nAnd the total revenue for the day is $ %.2f\n\n", shm_ptr->overall_client_num, total_revenue_generated);
					/* Write the statistic to the file*/
					fprintf(f_stats, "Total number of Clients who visited our restaurant is %i.\nAnd the total revenue for the day is $ %.2f\n\n", shm_ptr->overall_client_num, total_revenue_generated);

					printf("The most popular menu items with their respective revenue (Max five items)\n");
					fprintf(f_stats, "The most popular menu items with their respective revenue (Max five items)\n");
					/*Calculating top five most popular items*/
					int cur_max = menu_item_counts[0].menu_item_total_count;
					int cur_max_index = 0;
					int top_most_item_index = 1; // denoting the position of popular item i.e. 1 = 1st most popular
					while (top_most_item_index < 6 && top_most_item_index < menu_item_counts_index+1) {
						for (int i = 0; i < menu_item_counts_index; i++) {
							if (cur_max <= menu_item_counts[i].menu_item_total_count &&
							    menu_item_counts[i].chosen_for_top_five_already == 0) {
								cur_max_index = i;
								cur_max = menu_item_counts[i].menu_item_total_count;
							}
						}
						menu_item_counts[cur_max_index].chosen_for_top_five_already = 1;
						cur_max = 0;

						printf("%i. %s ordered %i times with a total revenue of $ %.2f.\n",
						       top_most_item_index, menu_item_counts[cur_max_index].menu_desc,
						       menu_item_counts[cur_max_index].menu_item_total_count, menu_item_counts[cur_max_index].menu_total_price );
						fprintf(f_stats, "%i. %s ordered %i times with a total revenue of $ %.2f.\n",
						        top_most_item_index, menu_item_counts[cur_max_index].menu_desc,
						        menu_item_counts[cur_max_index].menu_item_total_count, menu_item_counts[cur_max_index].menu_total_price );
						top_most_item_index  += 1;
					}
				}
				fclose(f_stats);
				////////////////////////* NORMAL EXIT CLEAN UP*/////////////////////////
				// close all sems and shm
				all_exit_cleanup(cashier_sem, cashier_cli_q_sem, deq_c_block_sem, server_sem,
				                 server_cli_q_sem, deq_s_block_sem, shm_write_sem, shutdown_sem,
				                 shm_ptr, &shm_fd);
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
		        "Max number of people serviceable (%li) cannot exceed global Maximum (%d)\n", *MaxPeople, MAX_REST_QUEUE_CAP);
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
	coordinator_only_exit_cleanup();
	exit(0);
}

/* Signal Handler for SIGTERM */
void sigterm_handler(int sig_num) {
	/* Reset handler to catch SIGTERM next time */
	signal(SIGTERM, sigint_handler);
	printf("\nGraceful termination and cleaning shm_mem after recv sigterm\n");
	fflush(stdout);

	/*EXIT CLEAN UP*/
	coordinator_only_exit_cleanup();
	exit(0);
}
