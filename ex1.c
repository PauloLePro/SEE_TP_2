//gcc -o ex1 ex1.c -lrt -lm ----- sudo ./ex1
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sched.h>
#include <errno.h>
#include <zconf.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#define PRIO "prio"
#define PERIODE 100

timer_t gTimerid;
int timer_iteration = 0;
struct timespec timings[PERIODE];
struct timespec result[PERIODE];

char *str_trim(char *str)
{
    int last_char = strlen(str) - 1; // we're starting from the last char
    int i = last_char;
    char c = str[i];

    char *ret = str;

    while (isspace(c) || (c == '\t'))
    { // because of course
        c = str[--i];
    }

    if (i != last_char)
        str[i + 1] = '\0'; // otherwise we erase the good last char

    i = 0;
    c = str[i];
    while (isspace(c) || (c == '\t'))
    {
        c = str[++i];
    }

    if (i != 0)
        ret += i;

    return ret;
}

int appel_sys_prio(pid_t pid)
{

    int which = PRIO_PROCESS;
    int ret;

    errno = 0;

    ret = getpriority(which, pid);
    if (errno != -1)
    {
        printf("getpriority = %d \n", ret);
    }
    else
    {
        perror("Error (getpriority) :");
    }

    return 0;
}

void prio_par_proc(void)
{

    FILE *file = NULL;
    char *line = NULL;
    char *saveline = NULL;
    ssize_t read;
    size_t len = 0;
    char *token;

    file = fopen("/proc/self/sched", "r");

    if (file != NULL)
    {

        while ((read = getline(&line, &len, file)) != -1)
        {
            token = strtok_r(line, ":", &saveline);
            line = str_trim(line);
            saveline = str_trim(saveline);

            if (!strcmp(token, PRIO))
            {
                printf("%s : %s \n", line, saveline);
            }
        }
        fclose(file);
    }
    else
    {
        perror("Error: ");
    }
}

int modif_prio(pid_t pid, int valprio)
{
    int which = PRIO_PROCESS;
    int ret;

    ret = setpriority(which, pid, valprio);
    if (ret == 0)
    {
        printf("setpriority = %d \n", valprio);
    }
    else
    {
        perror("Error (setpriority) :");
    }

    return 0;
}

void sched_fifo(void)
{
    int max_priority;
    struct sched_param param;

    /* Find out the MAX priority for the FIFO Scheduler */
    max_priority = sched_get_priority_max(SCHED_FIFO);

    /* Find out what the current priority is. */
    sched_getparam(0, &param);

    printf("La priorité actuel de param.sched_priority est de : %d \n", param.sched_priority);

    param.sched_priority = 20;

    if (param.sched_priority == -1)
    {
        perror("param.sched_priority :");
    }

    sched_setscheduler(0, SCHED_FIFO, &param);

    sched_getparam(0, &param);
    printf("La nouvelle valeur de param.sched_priority est de : %d \n", param.sched_priority);
}

void start_timer(void)
{
    struct itimerspec value;
    struct sigevent sev;

    value.it_value.tv_sec = 0;
    value.it_value.tv_nsec = 100000000;
    value.it_interval.tv_sec = value.it_value.tv_sec;
    value.it_interval.tv_nsec = value.it_value.tv_nsec;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &gTimerid;

    if (timer_create(CLOCK_REALTIME, &sev, &gTimerid) == -1)
    {
        perror("timer_create");
    }

    if (timer_settime(gTimerid, 0, &value, NULL) == -1)
    {
        perror("timer_settime");
    }
}

static void handler(int sig)
{
    printf("%d Signal capturé %d\n", timer_iteration, sig);
    clock_gettime(CLOCK_REALTIME, &timings[timer_iteration]);
    timer_iteration++;
    if (timer_iteration >= PERIODE)
    {
        timer_delete(gTimerid);
    }
}

long int moyenne(void)
{
    long int resultats = 0;

    for (int i = 0; i < PERIODE; i++)
    {
        resultats += (((result[i].tv_sec * 1000000) + (result[i].tv_nsec / 1000)) / PERIODE);
    }

    printf("Moyenne = %ld \n", resultats);

    return resultats;
}

void ecarts_types(long int moyenne)
{
    long int resultats = 0;
    long int diff[PERIODE];

    //calcul de l'écart type
    for (int i = 0; i < PERIODE; i++)
    {
        diff[i] = pow(((result[i].tv_sec * 1000000) + (result[i].tv_nsec / 1000)) - moyenne, 2);
        resultats += diff[i];
    }

    resultats = sqrt(resultats / PERIODE);

    printf("Ecart type = %ld \n", (resultats / 1000));
}

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

    printf("PID du processus : %d \n", pid);
    appel_sys_prio(pid);
    prio_par_proc();
    modif_prio(pid, 12);
    appel_sys_prio(pid);
    sched_fifo();

    signal(SIGRTMIN, handler);
    start_timer();
    while (1)
    {
        if (timer_iteration >= PERIODE)
        {
            break;
        }
    }

    for (int i = 0; i < (PERIODE - 1); i++)
    {
        result[i].tv_sec = timings[i + 1].tv_sec - timings[i].tv_sec;

        if (timings[i].tv_nsec > timings[i + 1].tv_nsec)
        {
            result[i].tv_sec--;
            result[i].tv_nsec = (1000000000 - timings[i].tv_nsec) + timings[i + 1].tv_nsec;
        }
        else
        {
            result[i].tv_nsec = timings[i + 1].tv_nsec - timings[i].tv_nsec;
        }

        printf("%d resultats : %ld \n", i, ((result[i].tv_sec * 1000000) + (result[i].tv_nsec / 1000)));
    }

    ecarts_types(moyenne());

    return 0;
}