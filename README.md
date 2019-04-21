# A restaurant application with IPC, shared memory and semaphores

This application demonstrates the concept of IPC, POSIX shared memory and semaphores. Real world applications not implemented in any way like this.

<img src="img/restaurant.png">

## Build (Linux)

```shell
$ make all
```

## Run

Only one `coordinator` and `server` should be invoked. However, multiple number of `cashiers` (Which is under `MaxNumOfCashiers`) and `clients` can be invoked. Each command arguments are discussed in the implementation below.

**Inside the `bin` folder run the following sequencially.**

```shell
$ ./coordinator -n MaxNumOfCashiers -p MaxPeople -t MaxTimeWait
$ ./server -m shmid
$ ./cashier -s serviceTime -b breakTime -m shmid
$ ./client -i itemId -e eatTime -m shmid
```

Data on individual clients and orders are saved in the `db` folder while the summary statistics such as `average waiting time for all customers`, `total visiting clients`, `total revenue` and `top five most popular dishes` are displayed when all clients have been served and the `coordinator` exits and saved in the `stats` folder as well.

### Clean build artifacts
```shell
$ make clean
```

### Notes on each program

**1)  Coordinator**

When the `coordinator` is invoked with the command below. It creates a shared memory and named semaphores under the POSIX standards, finally printing out the `shmid`. The other participants/processes access this shared memory through the `shmid`.

Note: The `shmid` is defined as global constant `SHMID` in `common.h`

```shell
$ ./coordinator -n MaxNumOfCashiers -p MaxPeople -t MaxTimeWait
```

-  `MaxNumOfCashiers` is the maximum number of cashiers that can be operating and invoked after the coordinator begins operation for the restaurant app.

-  `MaxPeople` is the maximum number of clients that can be present in the restaurant at any given time. If more clients attempt to join, they will simply exit.

-  `MaxTimeWait` is the maximum time that the restaurant waits before closing the shop given that no new clients have entered and all current clients have left the restaurant.

**2)  Server**

The `server` needs access to the menu data structure in the shared memory segment with id `shmid` initialized by the `coordinator`.

```shell
$ ./server -m shmid
```

**3)   Cashier**

A `cashier` checks the `MaxNumOfCashiers` value in the shared memory (**shm**) with id `shmid` initialized by the `coordinator`. The `cur_n_cashiers`in`shm`must be less than `MaxNumOfCashiers`.

```shell
$ ./cashier -s serviceTime -b breakTime -m shmid
```

-  `serviceTime` = maximum service time for dealing with one client
-  `breakTime` = maximum break time when the cashier_client_wait_queue is empty after which cashier returns back to station and goes back into break if there are no clients to serve again.

**4)   Client**

A `client` checks the `MaxPeople` value in the shared memory (**shm**) with id `shmid` initialized by the `coordinator`. The`cur_n_clients_wait_cashier`in`shm`must be less than `MaxPeople`. Otherwise `client` exits the restaurant.

```shell
$ ./client -i itemId -e eatTime -m shmid
```
-  `itemId` = id of item available in menu inside `db\diner_menu.txt`
-  `eatTime` = maximum eating time of the client

### Implementation

All the participating programs run on the following logic:

    /*Coordinator*/

    /*Cashier*/

    /*Client with Cashier*/

    /*Server*/

    /*Client with Server*/

The shared memory itself has the following structure, defined in `common.h`

```C
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
```

We load up the menu of available items from a `diner_menu.txt` file in the `db` folder.

To represent the menu items we use a custom C struct `Item`.

```C
typedef struct Item {
    int menu_itemId;
    char menu_desc[MAX_ITEM_DESC_LEN];
    int menu_price;
    int menu_min_time;
    int menu_max_time;
} Item;
```

To represent the client queue with cashiers we use a custom C struct `Client_CashQ_item`.

```C
typedef struct Client_CashQ_item {
    pid_t client_pid;
    int client_id;
    int menu_itemId;
} Client_CashQ_item;
```

To represent the client queue with the server we use a custom C struct `Client_ServQ_item`.

```C
typedef struct Client_ServQ_item {
    pid_t client_pid;
    int client_id;
    int menu_itemId;
} Client_ServQ_item;
```

And to represent the struct for storing client records, we use another custom C struct `Client_record_item`.

```C
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
```

### Author

-   <a href='https://www.linkedin.com/in/samridha-man-shrestha-89721412a/'>Samridha Shrestha</a>

### Acknowledgments

-   Linux man pages
