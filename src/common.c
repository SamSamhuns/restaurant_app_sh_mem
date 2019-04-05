/*  Author: Samridha Shrestha
    2019, April 2
 */
#include <ctype.h>
#include <stdio.h>
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
