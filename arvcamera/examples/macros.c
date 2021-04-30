#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s

#define ARV_GVCP_PORT(x) (3950 + x)

int main() {
    char port[10];
    sprintf(port, "%d", ARV_GVCP_PORT(10));
    printf("%s\n", port);
    return 0;
}
