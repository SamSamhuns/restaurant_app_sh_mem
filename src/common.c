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
		menu_items[menu_iter].menu_itemId = strtol(strtok(temp_loop_holder,","), NULL, 10);
		strcpy(menu_items[menu_iter].menu_desc, strtok(NULL, ","));
		menu_items[menu_iter].menu_price = strtol(strtok(NULL,","), NULL, 10);
		menu_items[menu_iter].menu_min_time = strtol(strtok(NULL,","), NULL, 10);
		menu_items[menu_iter].menu_max_time = strtol(strtok(NULL,","), NULL, 10);
		menu_iter += 1;
	}
}

/* FINAL CLEAN UP processed
    should only be called by the signal handlers
    in the coordinator */
void coordinator_exit_cleanup () {
	/* remove the named semaphores
	    IMPORTANT only coordinator should call this
	    at the end */
	sem_unlink(CASHIER_SEM);
	sem_unlink(CLIENTQ_SEM);
	sem_unlink(SHARED_MEM_WR_LOCK_SEM);
	/* remove the shared mem object
	    IMPORTANT only coordinator should call this
	    at the end */
	shm_unlink(SHMID);
}
