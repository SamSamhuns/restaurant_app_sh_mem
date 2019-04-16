#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

int main(int argc, char const *argv[]) {
	/* cmd args validation
	    ./server -m shmid */
	if ( argc != 3 || strcmp(argv[1], "-m") != 0 || isdigit_all(argv[2], strlen(argv[2]))) {
		fprintf(stderr,
		        "Incorrect args supplied. Usage: ./server -m shmid\n");
		return 1;
	}
	long shmid;
	shmid = strtol(argv[2], NULL, 10);
	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH(menu_file, "fopen_error");
	if (DEBUG == 1) printf("DEBUG Long is %li \n", shmid);

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];

	/* read the header */
	char temp_hdr[MAX_ITEM_DESC_LEN];
	fgets(temp_hdr, MAX_ITEM_DESC_LEN, menu_file);

	load_item_struct_arr(menu_file, menu_items);
	if (DEBUG) {
		int temp_num = 0;
		printf("%li %s %li %li %li\n", menu_items[temp_num].menu_itemId,
		       menu_items[temp_num].menu_desc, menu_items[temp_num].menu_price,
		       menu_items[temp_num].menu_min_time, menu_items[temp_num].menu_max_time
		       );
	}

	fclose(menu_file);

	/*TODO*/
	// get shmid and access it
	// check if cur_n_clients_wait_server > 0 then
	//		serve client FIFO from server_service_queue
	//      Both client and server locked during servicetime

	return 0;
}
