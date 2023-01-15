#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>

sem_t sem_read, sem_analyze, sem_print;
pthread_t reader_thread;
pthread_t analyzer_thread;
pthread_t printer_thread;

double* core_usage;
char** cpu_data;
int core_amount;

volatile sig_atomic_t done = 0;

void term(int signum){
    done = 1;
    pthread_cancel(reader_thread);
    pthread_cancel(analyzer_thread);
    pthread_cancel(printer_thread);
    free(core_usage);
    for(int i = 0; i <core_amount; i++) {
        free(cpu_data[i]);
    }
    free(cpu_data);
}

void* reader() {
    while (1) {
        sem_wait(&sem_read);
        int n = sysconf(_SC_NPROCESSORS_ONLN);
        FILE *stat_file = fopen("/proc/stat", "r");
        if (stat_file == NULL) {
            assert(stat_file==NULL);          
        }

        for(int i = 0; i < n; i++) {
            if (fgets(cpu_data[i], 1024, stat_file) != NULL) {
                if (strncmp(cpu_data[i], "cpu", 3) == 0) {
                } else break;
            }
        }
        fclose(stat_file);
        sem_post(&sem_analyze);
    }

    return NULL;
}

void* analyzer() {

    long long a =0;
    long long b=0;
    core_amount = sysconf(_SC_NPROCESSORS_ONLN);
    while (1) {
        sem_wait(&sem_analyze);
        int cpu_nr, user, nice, system, idle, iowait, irq, softirq, steal;
        for( int i =0; i<core_amount;i++){
            sscanf(cpu_data[i], "cpu %d %d %d %d %d %d %d %d %d",&cpu_nr, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            a =(user + nice + system + irq + softirq + steal);
            b =(user + nice + system + idle + iowait + irq + softirq + steal) ;
            core_usage[i]=(double)a/(double)b * 100.0;
        }
        sem_post(&sem_print);   
    }
        
    return NULL;
}

void* printer() {

    while (1) {
        sem_wait(&sem_print);
        double avg_cpu = 0 ;      
        for( int i =1; i<=core_amount;i++){
            avg_cpu += core_usage[1];
        }
        avg_cpu /=core_amount;
        printf("Total CPU usage: %f%%\n", avg_cpu);
        avg_cpu = 0 ;

        sleep(1);
        sem_post(&sem_read);
      }

    return NULL;
}
int main() {

    sem_init(&sem_read, 0, 1);
    sem_init(&sem_analyze, 0, 0);
    sem_init(&sem_print, 0, 0);

    int core_amount = sysconf(_SC_NPROCESSORS_ONLN);
    core_usage = (double*) malloc(core_amount * sizeof(double));
    if(core_usage == NULL){printf("Memory unavaiable");
        return 1;
    }
    cpu_data = (char**) malloc(core_amount * sizeof(char*));
    if(cpu_data == NULL){
        printf("Memory unavaiable");
        free(core_usage);
        return 1;
    }
    for(int i = 0; i <core_amount; i++) {
        cpu_data[i] = (char*) malloc(1024 * sizeof(char));
        if(cpu_data[i] == NULL){
            printf("Memory unavaiable");
            free(core_usage);
            for(int j = 0; j < i; j++) {
                free(cpu_data[j]);
            }
            free(cpu_data);
            return 1;
        }
    }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
        
    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&analyzer_thread, NULL, analyzer, NULL);
    pthread_create(&printer_thread, NULL, printer, NULL);

    pthread_join(reader_thread, NULL);
    pthread_join(analyzer_thread, NULL);
    pthread_join(printer_thread, NULL);

return 0;

}
