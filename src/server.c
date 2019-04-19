#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
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

	// if (DEBUG == 1) printf("DEBUG Long is %li \n", shmid);

	FILE *menu_file = fopen("./db/diner_menu.txt", "r");
	TRY_AND_CATCH_NULL(menu_file, "fopen_error");

	// Create a Item struct array to hold each item from diner menu
	struct Item menu_items[num_menu_items(menu_file)];
	load_item_struct_arr(menu_file, menu_items);
	fclose(menu_file);

	sem_t *clientQS = sem_open(CLIENTQ_SEM, 0); /* open existing clientQS semaphore */
	TRY_AND_CATCH_SEM(clientQS, "sem_open()");
	sem_t *cashierS = sem_open(CASHIER_SEM, 0); /* open existing cashierS semaphore */
	TRY_AND_CATCH_SEM(cashierS, "sem_open()");

	/* shared memory file descriptor */
	int shm_fd;

	/* pointer to shared memory object */
	void* ptr;

	/* open the shared memory object */
	shm_fd = shm_open(argv[2], O_RDONLY, 0666);
	TRY_AND_CATCH_INT(shm_fd, "shm_open()");

	/* memory map the shared memory object */
	ptr = mmap(0, MAX_SHM_SIZE,
	           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	int cc;
	printf("Scanning an int for syn\n");
	scanf("%d",&cc);
	/* read from the shared memory object */
	printf("%s", (char*)ptr);

	/* close the named semaphores */
	sem_close(clientQS);
	sem_close(cashierS);

	/* remove the shared memory object */
	munmap(ptr, MAX_SHM_SIZE);
	close(shm_fd);
	/* Server should not delete the shared mem object
	    shm_unlink(shmid);*/



	/*TODO*/
	// get shmid and access it
	// check if cur_n_clients_wait_server > 0 then
	//		serve client FIFO from server_service_queue
	//      Both client and server locked during servicetime

	return 0;
}
