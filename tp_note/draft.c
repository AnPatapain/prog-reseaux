#include <stdio.h>

int main(void)
{
    int i, j, n;
    
    char buffer[] = "\033[42m wrong password or you were not registered client\033[m";
    
    printf("\n%s\n", buffer);
    for (i = 0; i < 11; i++) {
        for (j = 0; j < 10; j++) {
            n = 10 * i + j;
            if (n > 108) break;
            printf("\033[%dm %3d\033[m", n, n);
        }
        printf("\n");
    }
    return 0;
}

