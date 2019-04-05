#ifndef COMMON_H // Making sure the header files are not included twice
#define COMMON_H

#define MAX_ITEM_DESC_LEN 125

/* struct representing each menu item as Item
    an array can be created using struct Item menuItemList[num_of_items] */
typedef struct Item {
	int menu_itemId;
	char menu_desc[MAX_ITEM_DESC_LEN];
	int menu_price;
	int menu_min_time;
	int menu_max_time;
} Item;

/* Checks if the string str is all digits
    returns 0 for True (str is all digits) and 1 for False (Not a number) */
int isdigit_all (const char *str, int str_len);

#endif
