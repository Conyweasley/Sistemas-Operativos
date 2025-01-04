#include <stdlib.h>  
#include <string.h>  
#include <assert.h>   
#include "strextra.h" 

char* strmerge(char* original, const char* addition) {
    if (original == NULL) {
        original = (char*)malloc(strlen(addition) + 1);
        if (original == NULL) {
            return NULL;
        }
        strcpy(original, addition);
    } else {
        size_t newLength = strlen(original) + strlen(addition) + 1;
        char* temp = (char*)realloc(original, newLength);
        if (temp == NULL) {
            return NULL;
        }
        strcat(temp, addition);
        original = temp;
    }

    return original;
}