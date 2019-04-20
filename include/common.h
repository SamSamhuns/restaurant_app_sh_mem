#ifndef COMMON_H // Making sure the header files are not included twice
#define COMMON_H

#define MAX_ITEM_DESC_LEN 125 // max len of menu items
#define MAX_SHMID_LEN 30 // Max length of shmid
#define MAX_REST_QUEUE_CAP 50 // Max size of our client and server queue, Max number of clients restaurant can handle in one run / day
#define SHMID "/0001_shm" // shared memory id
#define CASHIER_SEM "/cashier_sem" // named cashier semaphore
#define CLIENTQ_SEM "/clientQ_sem" // amed client queue semaphore
#define SHARED_MEM_WR_LOCK_SEM "/shared_mem_write_sem" // semaphore to lock the write segment

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
	long menu_itemId;
	char menu_desc[MAX_ITEM_DESC_LEN];
	long menu_price;
	long menu_min_time;
	long menu_max_time;
} Item;

typedef struct Client_CashQ_item {
    pid_t client_pid;
    int client_id;
    int menu_itemId;
} Client_CashQ_item;

typedef struct Client_ServQ_item {
    pid_t client_pid;
    int client_id;
    int menu_itemId;
} Client_ServQ_item;

/* This is the main  client record struct item */
typedef struct Client_record_item {
    pid_t clien_pid;
    int client_id;
    int menu_itemId;
    char menu_desc[MAX_ITEM_DESC_LEN];
    int menu_price;
    int eat_time;
    int time_with_cashier;
    int time_with_server;
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
    struct Client_CashQ_item client_cash_queue[MAX_REST_QUEUE_CAP];

    int front_server_Q;
    int rear_server_Q;
    int size_server_Q;
    struct Client_ServQ_item client_server_queue[MAX_REST_QUEUE_CAP];

    /* client_record_cur_size cannot exceed MAX_REST_QUEUE_CAP */
    int client_record_cur_size;
    struct Client_record_item client_record_queue[MAX_REST_QUEUE_CAP];

    /* static constants */
    int MaxCashiers;
    int MaxPeople;

    /* dynamic values */
    pid_t server_pid; // pid of current server program
    int totalCashierNum; // current number of cashieers
    int totalClientNum; // current number of total clients
    int totalClientOverall; // overall number of clients processed must be less than MaxPeople
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

/* FINAL CLEAN UP function
    should only be called by the signal handlers
    in the coordinator */
void coordinator_exit_cleanup ();

#endif
