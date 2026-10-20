#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Поток 1: захватывает mutex1, затем пытается захватить mutex2
void* thread1_func(void* arg) {
    printf("Thread 1: пытаюсь захватить mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: захватил mutex1\n");
    
    usleep(100000);  
    
    printf("Thread 1: пытаюсь захватить mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 1: захватил mutex2\n");
    
    printf("Thread 1: выполняю работу...\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Поток 2: захватывает mutex2, затем пытается захватить mutex1
void* thread2_func(void* arg) {
    printf("Thread 2: пытаюсь захватить mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: захватил mutex2\n");
    
    usleep(100000); 
    
    printf("Thread 2: пытаюсь захватить mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2: захватил mutex1\n");
    
    // Критическая секция
    printf("Thread 2: выполняю работу...\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    if (pthread_create(&thread1, NULL, thread1_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    if (pthread_create(&thread2, NULL, thread2_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("\n=== Программа завершилась (deadlock не произошел) ===\n");
    
    return 0;
}