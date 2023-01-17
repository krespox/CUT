#include "main.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>

volatile sig_atomic_t done = 0;
static sem_t sem_read, sem_analyze, sem_print;
static pthread_t reader_thread;
static pthread_t analyzer_thread;
static pthread_t printer_thread;
static pthread_t watchdog_thread;

static double* core_usage;
static char** cpu_data;
static long core_amount;
static int working = 0;

static void term(int signo){
    done = 1;
    pthread_cancel(reader_thread);
    pthread_cancel(analyzer_thread);
    pthread_cancel(printer_thread);
    pthread_cancel(watchdog_thread);
    printf("signal %d received\n",signo);
    free(core_usage);
    for(int i = 0; i < core_amount; i++) {
        free(cpu_data[i]);
    }
    free(cpu_data);
}

void* reader(void* unused) {       
    while (1) {
        FILE *stat_file = fopen("/proc/stat", "r");
        (void)unused;
        sem_wait(&sem_read);
        core_amount = sysconf(_SC_NPROCESSORS_ONLN);

        if (stat_file == NULL) {
            assert(stat_file==NULL);          
        }

        for(int i = 0; i < core_amount; i++) {
            if (fgets(cpu_data[i], 1024, stat_file) != NULL) {
                if (strncmp(cpu_data[i], "cpu", 3) == 0) {
                } else break;
            }
        }
        fclose(stat_file);
        working = 1;
        sem_post(&sem_analyze);
    }

    return NULL;
}

void* analyzer(void* unused) {
    int cpu_nr, user, nice, system, idle, iowait, irq, softirq, steal;
    long long a = 0;
    long long b = 0;
    core_amount = sysconf(_SC_NPROCESSORS_ONLN);
    (void)unused;
    while (1) {
        sem_wait(&sem_analyze);
        for( int i =0; i<core_amount;i++){
            sscanf(cpu_data[i], "cpu %d %d %d %d %d %d %d %d %d",&cpu_nr, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            a =(user + nice + system + irq + softirq + steal);
            b =(user + nice + system + idle + iowait + irq + softirq + steal) ;
            core_usage[i]=(double)a/(double)b * 100.0;
        }
    working = 1;
    sem_post(&sem_print);
}
return NULL;
}

void* printer(void* unused){
double avg_cpu = 0;
(void)unused;
while (1){
    sem_wait(&sem_print);

    for( int i =1; i<=core_amount;i++){
        avg_cpu += core_usage[i];
    }
    avg_cpu /= (double)core_amount;
    printf("Total CPU usage: %f%%\n", avg_cpu);
    avg_cpu = 0;
    sleep(1);
    working = 1;
    sem_post(&sem_read);
  }
return NULL;
}

static void* watchdog(void* unused){
    (void)unused;
    while(1){
        sleep(2);
        if(!working){
        printf("Error: All threads have stopped working.\n");
        term(SIGTERM);
        exit(EXIT_FAILURE);
        }
        working = 0;
    }
return NULL;
}

int main(void){
    struct sigaction action;
    sem_init(&sem_read, 0, 1);
    sem_init(&sem_analyze, 0, 0);
    sem_init(&sem_print, 0, 0);

    core_amount = sysconf(_SC_NPROCESSORS_ONLN);
    core_usage = (double*) malloc((unsigned long)core_amount * sizeof(double));
    cpu_data = (char**) malloc((unsigned long)core_amount * sizeof(char*));
    for(int i = 0; i < core_amount; i++){
        cpu_data[i] = (char*) malloc(1024 * sizeof(char));
    }

    action.sa_handler = term;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&analyzer_thread, NULL, analyzer, NULL);
    pthread_create(&printer_thread, NULL, printer, NULL);
    pthread_create(&watchdog_thread, NULL, watchdog, NULL);

    sem_post(&sem_read);
    pthread_join(reader_thread, NULL);
    pthread_join(analyzer_thread, NULL);
    pthread_join(printer_thread, NULL);
    pthread_join(watchdog_thread, NULL);

    sem_destroy(&sem_read);
    sem_destroy(&sem_analyze);
    sem_destroy(&sem_print);

    return 0;

}

