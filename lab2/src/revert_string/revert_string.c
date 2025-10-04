#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	if (str == NULL) return;
    
    int length = strlen(str);
    int i = 0;
    int j = length - 1;
    
    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        
        i++;
        j--;
    }
}

