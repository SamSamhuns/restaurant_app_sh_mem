#ifndef COMMON_H // Making sure the header files are not included twice
#define COMMON_H

#define MAX_ITEM_DESC_LEN 125 // max len of menu items
#define MAX_SHMID_LEN 30 // Max length of shmid
#define MAX_REST_QUEUE_CAP 50 // Max size of our client and server queue, Max number of clients restaurant can handle in one run / day
#define MAX_CASHIER_CAP 50 // Global max on number of cashiers despite another limit on number of cashiers
#define SHMID "0001" // shared memory id
#define NO_SERVER_TEMP_PID 1000000 // place holder server_pid
/*defining all semaphores */
#define CASHIER_SEM "/cashier_sem" 				// named cashier semaphore
#define CASHIER_CLI_Q_SEM "/cashier_cli_q_sem"  // named cashier client queue semaphore
#define DEQ_C_BLOCK_SEM "/deq_c_block_sem" 		// to block cashier from dequeueing before client reads its PID from cashier_pid_queue
#define SERVER_SEM "/server_sem" 				// named server semaphore
#define SERVER_CLI_Q_SEM "/server_cli_q_sem"	// named serverclient queue semaphore
#define DEQ_S_BLOCK_SEM "/deq_s_block_sem" 		// to block client from dequeueing before client reads its PID from client_server_queue
#define SHM_WRITE_SEM "/shm_write_sem" 			// semaphore to lock the sh memory during write operations
#define SHUTDOWN_SEM "/shutdown_sem"			// semaphore to block or allow shutdown initiation of restaurant

#define DEBUG 1 // Debug Mode

/* Boilerplate code for uniform error checking
   Example.  if(pipe(fd_pipe) == -1){
                perror("could not pipe");
                exit(1);
            }
   SHortened to:
            TRY_AND_CATCH(pipe(fd_pipe), "pipe");
 */
#define TRY_AND_CATCH_INT(exp, err_cmd) do { \
		if (exp < 0) { \
			perror(err_cmd); \
			exit(1);   \
		} \
} while (0)

/* For catching NULL pointers*/
#define TRY_AND_CATCH_NULL(exp, err_cmd) do { \
		if (exp == NULL) { \
			perror(err_cmd); \
			exit(1);   \
		} \
} while (0)

/* For catching sem_open erros */
#define TRY_AND_CATCH_SEM(sem_exp, err_cmd) do { \
		if (sem_exp == SEM_FAILED) { \
			perror(err_cmd); \
			exit(1);   \
		} \
} while (0)

/* struct representing each menu item as Item
    an array can be created using struct Item menuItemList[num_of_items] */
typedef struct Item {
	long menu_item_id;
	char menu_desc[MAX_ITEM_DESC_LEN];
	long menu_price;
	long menu_min_time;
	long menu_max_time;
} Item;

typedef struct Client_Queue_item {
	pid_t client_pid;
	int menu_item_id;
} Client_Queue_item;

/* This is the main client record struct item */
typedef struct Client_record_item {
	pid_t client_pid;
	int menu_item_id;
	char menu_desc[MAX_ITEM_DESC_LEN];
	int menu_price;
	int eat_time;
	int time_with_cashier;
	int time_with_server;
	int total_time_spent;
} Client_record;

/*Creating a shared memory struct*/
typedef struct Shared_memory_struct {
	/* We will deploy three custom array based queues with a
	    maximum capacity of MAX_REST_QUEUE_CAP (The maximum num of clients the
	    restaurant can handle )
	    They are client cashier queue, client server queue and a client record queue */
	int front_client_Q;
	int rear_client_Q;
	int size_client_Q;
	struct Client_Queue_item client_cashier_queue[MAX_REST_QUEUE_CAP];

	int front_server_Q;
	int rear_server_Q;
	int size_server_Q;
	struct Client_Queue_item client_server_queue[MAX_REST_QUEUE_CAP];

	/* client_record_cur_size cannot exceed MAX_REST_QUEUE_CAP */
	int cur_client_record_size;
	struct Client_record_item client_record_array[MAX_REST_QUEUE_CAP];

	/* static constants */
	int MaxCashiers;
	int MaxPeople;

	/* dynamic values */
	int initiate_shutdown; // 1 is all processes should initiate shutdown
	pid_t cashier_pid_array[MAX_CASHIER_CAP]; // array of pids of cashier processes
	pid_t server_pid; // pid of current server program
	int cur_cashier_num; // current number of cashieers
	int cur_client_num; // current number of total clients
	int overall_client_num; // overall number of clients processed must be less than MaxPeople
} Shared_memory_struct;

/* Checks if the string str is all digits
    returns 0 for True (str is all digits) and 1 for False (Not a number) */
int isdigit_all (const char *str, int str_len);

/* function that returns the total number of items in menu to create
    menu struct array*/
int num_menu_items(FILE *fptr);

/* function that parses the menu item file and loads each item
    with id, name, price with min and max time for waiting */
void load_item_struct_arr(FILE *menu_file, struct Item menu_items[]);

/* FINAL CLEAN UP function for unlinking shm and sem
    should only be called in the coordinator */
void coordinator_only_exit_cleanup ();

/* Normal exit clean up call for all processes */
void all_exit_cleanup (sem_t *clientQS,
                       sem_t *cashierS,
                       sem_t *shm_write_sem,
                       struct Shared_memory_struct *shm_ptr,
                       int *shm_fd);

/* func to enqueue client pid in cashier FIFO queue
    This function automatically does a shared mem write semaphore lock
    IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void enqueue_client_cashier_q(struct Shared_memory_struct *shm_ptr,
                              long menu_item_id, sem_t *shm_write_sem);

/* func to enqueue client pid in server FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void enqueue_client_server_q(struct Shared_memory_struct *shm_ptr,
                             long menu_item_id, sem_t *shm_write_sem);

/* func to dequeue client pid item in client FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void dequeue_client_cashier_q(struct Shared_memory_struct *shm_ptr, sem_t *shm_write_sem);

/* func to dequeue client pid item in server FIFO queue
   This function automatically does a shared mem write semaphore lock
   IMPORTANT: This function should NEVER be called INSIDE the shared mem write semaphore lock state */
void dequeue_client_server_q(struct Shared_memory_struct *shm_ptr, sem_t *shm_write_sem);


#endif
