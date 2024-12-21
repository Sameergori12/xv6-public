#include "types.h"
 #include "stat.h"
 #include "user.h"
 int main() {
    int pid = fork();
    if(pid < 0) {   // Child 1
        printf(1, "Error forking child 1.\n"); 
    } else if (pid == 0) {
        printf(1, "Child 1 Executing\n"); 

    } else {        
        pid = fork();   // Child 2
        if(pid < 0) {
            printf(1, "Error forking second child.\n"); 
        } else if(pid == 0) {
            printf(1, "Child 2 Executing\n"); 
        } else {    // Parent
            printf(1, "Parent Waiting\n"); 
            wait();
            wait();
        printf(1, "Children Exited\n"); 
        printf(1, "Parent Executing\n"); 
        printf(1, "Parent Exiting\n");
        } 
    }
    exit(); 
}