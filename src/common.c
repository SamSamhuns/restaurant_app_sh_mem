/*  Author: Samridha Shrestha
    2019, April 2
 */
#include <ctype.h>
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

/* Checks if the string str is all digits
    returns 0 for True (str is all digits) and 1 for False (Not a number) */
int isdigit_all (const char *str, int str_len) {
	for (int j = 0; j < str_len; j++) {
		if (isdigit(str[j]) == 0 ) {
			return 1; /*Not a digit*/
		}
	}
	return 0;
}

/* function that returns the total number of items in menu to create
    menu struct array*/
int num_menu_items(FILE *fptr) {
	int numberItems = 0;
	char temp_rd_buffer[MAX_ITEM_DESC_LEN];
	while (fgets(temp_rd_buffer, MAX_ITEM_DESC_LEN, fptr)) {
		numberItems += 1;
	}
	fseek(fptr, 0, SEEK_SET); // reset file ptr to beginning of file
	return numberItems-1; // header column is not counted
}

/* function that parses the menu item file and loads each item
    with id, name, price with min and max time for waiting */
void load_item_struct_arr(FILE *menu_file, struct Item menu_items[]) {
	/* read the header to prevent re-reading it*/
	char temp_hdr[MAX_ITEM_DESC_LEN];
	fgets(temp_hdr, MAX_ITEM_DESC_LEN, menu_file);

	char temp_loop_holder[MAX_ITEM_DESC_LEN];
	int menu_iter=0;
	while (fgets(temp_loop_holder, MAX_ITEM_DESC_LEN, menu_file)) {
		menu_items[menu_iter].menu_item_id = strtol(strtok(temp_loop_holder,","), NULL, 10);
		strcpy(menu_items[menu_iter].menu_desc, strtok(NULL, ","));
		menu_items[menu_iter].menu_price = strtol(strtok(NULL,","), NULL, 10);
		menu_items[menu_iter].menu_min_time = strtol(strtok(NULL,","), NULL, 10);
		menu_items[menu_iter].menu_max_time = strtol(strtok(NULL,","), NULL, 10);
		menu_iter += 1;
	}
}

/* FINAL CLEAN UP function for unlinking shm and sem
    should only be called in the coordinator */
void coordinator_only_exit_cleanup() {
	/* remove the named semaphores*/
	TRY_AND_CATCH_INT(sem_unlink(CASHIER_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(CASHIER_CLI_Q_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(DEQ_C_BLOCK_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(SERVER_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(SERVER_CLI_Q_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(DEQ_S_BLOCK_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(SHUTDOWN_SEM), "sem_unlink");
	TRY_AND_CATCH_INT(sem_unlink(SHM_WRITE_SEM), "sem_unlink");

	/* remove the shared mem object */
	TRY_AND_CATCH_INT(shm_unlink(SHMID), "shm_unlink");
}

/* Normal exit semaphore close and mem unmap call
    This will be called only by the coordinator and the client since they use
    all semaphores */
void all_exit_cleanup(sem_t *cashier_sem,
                      sem_t *cashier_cli_q_sem,
                      sem_t *deq_c_block_sem,
                      sem_t *server_sem,
                      sem_t *server_cli_q_sem,
                      sem_t *deq_s_block_sem,
                      sem_t *shm_write_sem,
                      sem_t *shutdown_sem,
                      struct Shared_memory_struct *shm_ptr,
                      int *shm_fd) {
	/* close the named semaphores with error handling */
	TRY_AND_CATCH_INT(sem_close(cashier_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(cashier_cli_q_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(deq_c_block_sem), "sem_close");

	TRY_AND_CATCH_INT(sem_close(server_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(server_cli_q_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(deq_s_block_sem), "sem_close");

	TRY_AND_CATCH_INT(sem_close(shm_write_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(shutdown_sem), "sem_close");

	/* remove the shared memory object with error handling */
	TRY_AND_CATCH_INT(
		munmap(shm_ptr, sizeof(Shared_memory_struct)), "munmap()");
	close(*shm_fd);
	printf("Successfully exiting process with pid %li\n", (long)getpid());
}

/* Normal exit sempaphore and shm cleanup for cashier and the sems it uses */
void cashier_exit_cleanup(sem_t *cashier_sem,
                          sem_t *cashier_cli_q_sem,
                          sem_t *deq_c_block_sem,
                          sem_t *shm_write_sem,
                          sem_t *shutdown_sem,
                          struct Shared_memory_struct *shm_ptr,
                          int *shm_fd) {
	/* close the named semaphores with error handling */
	TRY_AND_CATCH_INT(sem_close(cashier_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(cashier_cli_q_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(deq_c_block_sem), "sem_close");

	TRY_AND_CATCH_INT(sem_close(shm_write_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(shutdown_sem), "sem_close");

	/* remove the shared memory object with error handling */
	TRY_AND_CATCH_INT(
		munmap(shm_ptr, sizeof(Shared_memory_struct)), "munmap()");
	close(*shm_fd);
	printf("Successfully exiting process with pid %li\n", (long)getpid());
}

/* Normal exit sempaphore and shm cleanup for server and the sems it uses */
void server_exit_cleanup(sem_t *server_sem,
                         sem_t *server_cli_q_sem,
                         sem_t *deq_s_block_sem,
                         sem_t *shm_write_sem,
                         sem_t *shutdown_sem,
                         struct Shared_memory_struct *shm_ptr,
                         int *shm_fd) {
	/* close the named semaphores with error handling */
	TRY_AND_CATCH_INT(sem_close(server_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(server_cli_q_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(deq_s_block_sem), "sem_close");

	TRY_AND_CATCH_INT(sem_close(shm_write_sem), "sem_close");
	TRY_AND_CATCH_INT(sem_close(shutdown_sem), "sem_close");

	/* remove the shared memory object with error handling */
	TRY_AND_CATCH_INT(
		munmap(shm_ptr, sizeof(Shared_memory_struct)), "munmap()");
	close(*shm_fd);
	printf("Successfully exiting process with pid %li\n", (long)getpid());
}

/* func to enqueue client pid in cashier FIFO queue
    This function automatically does a shared mem write semaphore lock
    IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void enqueue_client_cashier_q(struct Shared_memory_struct *shm_ptr,
                              long menu_item_id, sem_t *shm_write_sem) {
	////////* Acquire semaphore lock first before writing in shared memory *////
	////////////////////////////////////////////////////////////////////////////
	TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
	/* if no clients have been processed before in the cashier queue */
	if (shm_ptr->size_client_Q < 0) {
		shm_ptr->client_cashier_queue[0].client_pid = getpid();
		shm_ptr->client_cashier_queue[0].menu_item_id = (int)menu_item_id;
		shm_ptr->front_client_Q = 0;         // front = 0
		shm_ptr->rear_client_Q = 0;         // rear = 0
		shm_ptr->size_client_Q = 1;         // size  = 1
	}
	/* if clients have been previously processed in the cashier queue */
	else {
		int rear = shm_ptr->rear_client_Q;
		shm_ptr->client_cashier_queue[rear+1].client_pid = getpid();
		shm_ptr->client_cashier_queue[rear+1].menu_item_id = (int)menu_item_id;
		shm_ptr->rear_client_Q++;       // rear ++
		shm_ptr->size_client_Q++;       // size ++
	}
	/* release semaphore write lock after writing to shared memory */
	TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
	///////////////////////////////////////////////////////////////////////////
}

/* func to enqueue client pid in server FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void enqueue_client_server_q(struct Shared_memory_struct *shm_ptr,
                             long menu_item_id, sem_t *shm_write_sem) {
	////////* Acquire semaphore lock first before writing in shared memory *////
	////////////////////////////////////////////////////////////////////////////
	TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
	/* if no clients have been processed before in the cashier queue */
	if (shm_ptr->size_server_Q < 0) {
		/* Add client pid and order info to queue */
		shm_ptr->client_server_queue[0].client_pid = getpid();
		shm_ptr->client_server_queue[0].menu_item_id = (int)menu_item_id;
		shm_ptr->front_server_Q = 0;         // front = 0
		shm_ptr->rear_server_Q = 0;         // rear = 0
		shm_ptr->size_server_Q = 1;         // size  = 1
	}
	/* if clients have been previously processed in the cashier queue */
	else {
		int rear = shm_ptr->rear_server_Q;
		/* Add client pid and order info to queue */
		shm_ptr->client_server_queue[rear+1].client_pid = getpid();
		shm_ptr->client_server_queue[rear+1].menu_item_id = (int)menu_item_id;
		shm_ptr->rear_server_Q++;       // rear ++
		shm_ptr->size_server_Q++;       // size ++
	}
	/* release semaphore write lock after writing to shared memory */
	TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
	///////////////////////////////////////////////////////////////////////////
}

/* func to dequeue client pid item in client FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void dequeue_client_cashier_q(struct Shared_memory_struct *shm_ptr, sem_t *shm_write_sem) {
	////////* Acquire semaphore lock first before writing in shared memory *////
	////////////////////////////////////////////////////////////////////////////
	TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
	if (shm_ptr->size_client_Q > 0) {
		shm_ptr->size_client_Q--;      // size --
		shm_ptr->front_client_Q++;      // front ++
	}
	else {
		fprintf(stderr, "Cashier client queue is empty. Cannot dequeue\n");
	}
	/* release semaphore write lock after writing to shared memory */
	TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
	///////////////////////////////////////////////////////////////////////////
}

/* func to dequeue client pid item in server FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void dequeue_client_server_q(struct Shared_memory_struct *shm_ptr, sem_t *shm_write_sem) {
	////////* Acquire semaphore lock first before writing in shared memory *////
	////////////////////////////////////////////////////////////////////////////
	TRY_AND_CATCH_INT(sem_wait(shm_write_sem), "sem_wait()");
	if (shm_ptr->size_server_Q > 0) {
		shm_ptr->size_server_Q--;  // size --
		shm_ptr->front_server_Q++;  // front ++
	}
	else {
		fprintf(stderr, "Server client queue is empty. Cannot dequeue\n");
	}
	/* release semaphore write lock after writing to shared memory */
	TRY_AND_CATCH_INT(sem_post(shm_write_sem), "sem_post()");
	///////////////////////////////////////////////////////////////////////////
}
