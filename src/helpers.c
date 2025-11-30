#ifndef HELPERS_C
#define HELPERS_C

#include <string.h>

char *getFileExtension(char *strStart, int maxLen)
{
    // Maybe refactor this one day
    int pos;
    int isValid = 0;

    for (pos = 0; pos <= maxLen; pos++) {
        if (*(strStart + pos) == '\0') {
            isValid = 1;
            break;
        }
    }

    if (isValid) {
        isValid = 0;
        // Walkback till we find a .
        while (pos > 0) {
            if (*(strStart + pos) == '.') {
                isValid = 1;
                break;
            }
            pos--;
        }
    }

    return (isValid == 0) ? NULL : (strStart + pos);
}
#endif
