#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

pthread_t reader_thread;
pthread_t analyzer_thread;
pthread_t printer_thread;

pthread_mutex_t mutex;
pthread_cond_t cond;

char buffer[1024];
int total_cpu_usage;

void* reader(void* arg) {
    while (1) {
        FILE *stat_file = fopen("/proc/stat", "r");
        if (stat_file == NULL) {
            perror("Nie udało się otworzyć pliku /proc/stat");
            return NULL;
        }

        fgets(buffer, 1024, stat_file);
        fclose(stat_file);

        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

    
    }

    return NULL;
}

void* analyzer(void* arg) {
  long long a =0;
  long long b=0;
  double c =0;
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);

        int user, nice, system, idle, iowait, irq, softirq;
        sscanf(buffer, "cpu %d %d %d %d %d %d %d", &user, &nice, &system, &idle, &iowait, &irq, &softirq);
        a =(user + nice + system + iowait + irq + softirq)*100.0;
        b =(user + nice + system + idle + iowait + irq + softirq) ;
        total_cpu_usage=(double)a/(double)b;
        char output_buffer[16];
        sprintf(output_buffer, "%.2f%%", (double)total_cpu_usage);

        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        //printf("user = %d\n", user);
        //printf("nice = %d\n", nice);
        //printf("system = %d\n", system);
        //printf("idle = %d\n", idle);
        //printf("iowait = %d\n", iowait);
        //printf("irq = %d\n", irq);
        //printf("softirq = %d\n", softirq);
        //printf("total cpu = %d\n", total_cpu_usage);
    }

    return NULL;
}

void* printer(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);

        printf("Całkowite zużycie procesora: %d%%\n", total_cpu_usage);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }

    return NULL;
}
int main(int argc, char *argv[]) {
  
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&analyzer_thread, NULL, analyzer, NULL);
    pthread_create(&printer_thread, NULL, printer, NULL);

    pthread_join(reader_thread, NULL);
    pthread_join(analyzer_thread, NULL);
    pthread_join(printer_thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

