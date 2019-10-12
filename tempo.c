//gcc -o tempo tempo.c -lpthread

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{

    const int core_id = 1;
    const pid_t pid = getpid();

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    const int set_result = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);

    if (set_result != 0)
    {
        perror("sched_setaffinity :");
    }

    int nbgen = 0;

    while (1)
    {
        srand(time(NULL));
        nbgen = rand() % 9 + 1; //entre 1-9

        if (nbgen == 5)
        {
            sleep(1);
        }
    }

    return 0;
}