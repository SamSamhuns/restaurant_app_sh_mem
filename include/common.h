#ifndef COMMON_H // Making sure the header files are not included twice
#define COMMON_H

#define MAX_ITEM_DESC_LEN 125 // max len of menu items
#define MAX_SHMID_LEN 30 // Max length of shmid
#define CASHIER_SEM "/cashier_sem" // named cashier semaphore
#define CLIENTQ_SEM "/clientQ_sem" // amed client queue semaphore

#define DEBUG 1 // Debug Mode
#define MAX_SHM_SIZE 4096 // max size of shared memory in bytes

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

/*Creating a shared memory struct*/
typedef struct shared_memory_struct {
    int MaxCashiers;
    int MaxPeople;
    int closeRestaurant;
    int totalCashierNum;
    int totalClientNum;
} shared_memory_struct;

/* Checks if the string str is all digits
    returns 0 for True (str is all digits) and 1 for False (Not a number) */
int isdigit_all (const char *str, int str_len);

/* function that returns the total number of items in menu to create
	menu struct array*/
int num_menu_items(FILE *fptr);

/* function that parses the menu item file and loads each item
	with id, name, price with min and max time for waiting */
void load_item_struct_arr(FILE *menu_file, struct Item menu_items[]);

#endif
