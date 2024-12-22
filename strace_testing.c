#include <stdio.h>
int main(void) {
    FILE *filePointer;
    filePointer = fopen("README", "r");
    fprintf(filePointer, "Can we write to this file?\n");
    fclose(filePointer);
    return 0;
}
