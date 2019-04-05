# A restaurant application with IPC, shared memory and semaphores

This application demonstrates the concept of IPC, POSIX shared memory and semaphores. Real world applications not implemented in any way like this.

## Build (Linux)

```shell
$ make all
```

## Run

Only one `coordinator` and `server` should be invoked. However, multiple number of `cashiers` (Which is under `MaxNumOfCashiers`) and `clients` can be invoked. Each command arguments are discussed in the implementation below.

**Inside the `bin` folder run the following sequencially.**

```shell
$ ./coordinator -n MaxNumOfCashiers -p MaxPeople
$ ./server -m shmid
$ ./cashier -s serviceTime -b breakTime -m shmid
$ ./client -i itemId -e eatTime -m shmid
```

Data on individual clients and orders are saved in the `db` folder while the summary statistics such as `average waiting time for all customers`, `total visiting clients`, `total revenue` and `top five most popular dishes` are displayed when all clients have been served and the `coordinator` exits and saved in the `stats` folder as well.

### Clean build artifacts
```shell
$ make clean
```

### Implementation

**1)  Coordinator**

When the `coordinator` is invoked with the following command. It creates a shared segment of memory with id `shmid` and unnamed semaphores under the POSIX standards. The other players/processes access this same sahred memory.

```shell
$ ./coordinator -n MaxNumOfCashiers -p MaxPeople
```

-  `MaxNumOfCashiers` is the maximum number of cashiers that can be operating and invoked after the coordinator begins operation for the restaurant app.

-  `MaxPeople` is the maximum number of clients that can be waiting in the `FIFO` queue to be handled by `cashiers`. If more clients attempt to join, they will not join the client queue (process queue).

**2)  Server**

The `server` needs access to the menu data structure in the shared memory segment with id `shmid` initialized by the `coordinator`.

```shell
$ ./server -m shmid
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

### Author

-   <a href='https://www.linkedin.com/in/samridha-man-shrestha-89721412a/'>Samridha Shrestha</a>

### Acknowledgments

-   Linux man pages
