#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdbool.h>

#include "../../lab3/src/utils.h"
#include "sum_lib.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (true) {
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            threads_num = atoi(optarg);
            if (threads_num <= 0) { printf("threads_num must be > 0\n"); return 1; }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) { printf("array_size must be > 0\n"); return 1; }
            break;
          case 2:
            seed = atoi(optarg);
            if (seed <= 0) { printf("seed must be > 0\n"); return 1; }
            break;
        }
        break;
      default:
        break;
    }
  }

  if (threads_num == 0 || array_size == 0 || seed == 0) {
    printf("Usage: %s --threads_num num --array_size num --seed num\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
  struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);

  int chunk_size = array_size / threads_num;

  struct timeval start_time, finish_time;
  gettimeofday(&start_time, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == threads_num - 1) ? array_size : args[i].begin + chunk_size;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *res;
    pthread_join(threads[i], &res);
    total_sum += (int)(size_t)res;
  }

  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(threads);
  free(args);

  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);

  return 0;
}