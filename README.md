# A restaurant application with IPC, shared memory and semaphores

This application demonstrates the concept of IPC , shared memory and semaphores. Real world applications not implemented in any way like this.

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
