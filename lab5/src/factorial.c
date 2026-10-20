#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

long long result = 1;
int k, pnum, mod;
pthread_mutex_t mutex;

typedef struct {
    int start;
    int end;
    int id;
} thread_args;

//вычисляем часть факториала
void* compute_factorial(void* arg) {
    thread_args* args = (thread_args*)arg;
    long long partial = 1;
    
    // Вычисляем частичное произведение в диапазоне [start, end]
    for (int i = args->start; i <= args->end; i++) {
        partial = (partial * i) % mod;
    }
    
    pthread_mutex_lock(&mutex);
    result = (result * partial) % mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    k = 0;
    pnum = 0;
    mod = 0;
    
    // Парсинг аргументов командной строки
    static struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "k:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
                exit(1);
        }
    }
    
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: все параметры должны быть положительными числами\n");
        fprintf(stderr, "Пример: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
        exit(1);
    }
    
    pthread_mutex_init(&mutex, NULL);
    
    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    thread_args* args = malloc(pnum * sizeof(thread_args));
    
    // Вычисляем диапазон для каждого потока
    int range = k / pnum;
    int remainder = k % pnum;
    
    int start = 1;
    for (int i = 0; i < pnum; i++) {
        // Распределяем остаток по первым потокам
        int end = start + range - 1;
        if (i < remainder) {
            end++;
        }
        
        // Последний поток обрабатывает все оставшиеся числа
        if (i == pnum - 1) {
            end = k;
        }
        
        args[i].start = start;
        args[i].end = end;
        args[i].id = i;
        
        start = end + 1;
        
        if (pthread_create(&threads[i], NULL, compute_factorial, &args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
    
    // Освобождаем ресурсы
    free(threads);
    free(args);
    pthread_mutex_destroy(&mutex);
    
    // Выводим результат
    printf("%lld\n", result);
    
    return 0;
}