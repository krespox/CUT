#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

void* read_data_to_buffer(void* arg);
void* analyze_cores(void* arg);
void* print_cpu_avg(void* arg);


#define NUM_CORES 5

pthread_t reader_thread;
pthread_t analyzer_thread;
pthread_t printer_thread;

pthread_mutex_t mutex;
pthread_cond_t cond;
double core_usage[NUM_CORES];
char cpu_data[NUM_CORES][1024];
double cpu_usage[NUM_CORES];

void sigterm_handler(int signum) {

pthread_cancel(reader_thread);
pthread_cancel(analyzer_thread);
pthread_cancel(printer_thread);
exit(0);
}

void* read_data_to_buffer(void* arg) {
    while (1) {

        FILE *stat_file = fopen("/proc/stat", "r");
        if (stat_file == NULL) {
            assert(stat_file==NULL);          
        }

        int i = 0;
        while (fgets(cpu_data[i], 1024, stat_file) != NULL) {
            if (strncmp(cpu_data[i], "cpu", 3) == 0) {
               // printf("%s",cpu_data[i]);
                i++;
            } else break;
        }
        fclose(stat_file);


        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }

    return NULL;
}

void* analyze_cores(void* arg) {
  long long a =0;
  long long b=0;
  //double c =0;

    while (1) {

        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);

        int cpu_nr, user, nice, system, idle, iowait, irq, softirq, steal;
        for( int i =1; i<=NUM_CORES;i++){
            sscanf(cpu_data[i], "cpu %d %d %d %d %d %d %d %d %d",&cpu_nr, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            a =(user + nice + system + irq + softirq + steal);
            b =(user + nice + system + idle + iowait + irq + softirq + steal) ;
            core_usage[i]=(double)a/(double)b * 100.0;
           // printf("Core%d= %f\n",i -1 ,core_usage[i]);
        }
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&cond);
            // printf("a = %lld\n", a);
            //printf("b = %lld\n", b);
            //printf("system = %d\n", system);
            //printf("idle = %d\n", idle);
            //printf("iowait = %d\n", iowait);
            //printf("irq = %d\n", irq);
            //printf("softirq = %d\n", softirq);       
    }

    return NULL;
}

void* print_cpu_avg(void* arg) {
    while (1) {
   
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        double avg_cpu = 0 ;
        for( int i =1; i<=NUM_CORES;i++){
        avg_cpu += core_usage[i];
        
        }
        avg_cpu /=NUM_CORES;
        printf("Całkowite zużycie procesora: %f%%\n", avg_cpu);

        pthread_mutex_unlock(&mutex);
        sleep(1);
      }

    return NULL;
}
int main(int argc, char *argv[]) {
    signal(SIGTERM, sigterm_handler);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);


    pthread_create(&reader_thread, NULL, read_data_to_buffer, NULL);
    pthread_create(&analyzer_thread, NULL, analyze_cores, NULL);
    pthread_create(&printer_thread, NULL, print_cpu_avg, NULL);


    pthread_join(reader_thread, NULL);
    pthread_join(analyzer_thread, NULL);
    pthread_join(printer_thread, NULL);

  
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
