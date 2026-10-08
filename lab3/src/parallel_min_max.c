cat > parallel_min_max.c << 'EOF'
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
            return 1;
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }
  
  GenerateArray(array, array_size, seed);

  int pipes[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("Pipe creation failed");
        free(array);
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      if (child_pid == 0) {
        // Child process
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : start + chunk_size;

        struct MinMax min_max = GetMinMax(array, start, end);

        if (with_files) {
          char filename[64];
          snprintf(filename, sizeof(filename), "temp_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f) {
            fprintf(f, "%d %d\n", min_max.min, min_max.max);
            fclose(f);
          } else {
            perror("Failed to open file");
          }
        } else {
          close(pipes[i][0]);
          if (write(pipes[i][1], &min_max.min, sizeof(int)) != sizeof(int)) {
            perror("Failed to write min");
          }
          if (write(pipes[i][1], &min_max.max, sizeof(int)) != sizeof(int)) {
            perror("Failed to write max");
          }
          close(pipes[i][1]);
        }
        
        free(array);
        return 0;
      }
    } else {
      printf("Fork failed!\n");
      free(array);
      return 1;
    }
  }

  // Parent process - collect results
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int status;
    
    // Wait for specific child
    pid_t pid = wait(&status);
    if (pid == -1) {
      perror("wait failed");
      continue;
    }

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "temp_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        if (fscanf(f, "%d %d", &min, &max) != 2) {
          fprintf(stderr, "Failed to read from %s\n", filename);
        }
        fclose(f);
        remove(filename);
      } else {
        perror("Failed to open file for reading");
      }
    } else {
      close(pipes[i][1]);
      ssize_t bytes_read;
      bytes_read = read(pipes[i][0], &min, sizeof(int));
      if (bytes_read != sizeof(int)) {
        fprintf(stderr, "Failed to read min from pipe %d\n", i);
      }
      bytes_read = read(pipes[i][0], &max, sizeof(int));
      if (bytes_read != sizeof(int)) {
        fprintf(stderr, "Failed to read max from pipe %d\n", i);
      }
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  
  return 0;
}
EOF
