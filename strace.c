#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(void) {
    char *file_name = "README";
    int fd = open(file_name, O_CREATE | O_RDONLY);
    printf(1, "File Descriptor: %d\n", fd);

    // Attempt to write to the file
    int res = write(fd, "Test Write\n", -1);
    printf(1, "Write Result: %d\n", res);

    close(fd);
    exit();
}
