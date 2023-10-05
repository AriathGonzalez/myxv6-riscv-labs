#include "kernel/types.h"
#include "user/user.h"

char *argv[] = { "sh", 0 };
int main(void) {
    exec("ps", argv);
    exit(0);
}

