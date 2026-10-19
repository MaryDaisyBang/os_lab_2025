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
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

static pid_t *child_pids = NULL;
static int num_children = 0;

void alarm_handler(int sig) {
    printf("Timeout! Killing all child processes...\n");
    for (int i = 0; i < num_children; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
            printf("Sent SIGKILL to process %d\n", child_pids[i]);
        }
    }
}

int main(int argc, char **argv) {
    int seed = -1, array_size = -1, pnum = -1, timeout = 0;
    bool with_files = false;

    while (true) {
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: seed = atoi(optarg); if (seed <= 0) { printf("seed must be positive\n"); return 1; } break;
                    case 1: array_size = atoi(optarg); if (array_size <= 0) { printf("array_size must be positive\n"); return 1; } break;
                    case 2: pnum = atoi(optarg); if (pnum <= 0) { printf("pnum must be positive\n"); return 1; } break;
                    case 3: with_files = true; break;
                    case 4: timeout = atoi(optarg); if (timeout <= 0) { printf("timeout must be positive\n"); return 1; } break;
                    default: break;
                }
                break;
            case 'f': with_files = true; break;
            default: break;
        }
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed num --array_size num --pnum num [--by_files] [--timeout num]\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (!array) { printf("Memory allocation failed\n"); return 1; }
    GenerateArray(array, array_size, seed);

    int pipes[pnum][2];
    child_pids = malloc(sizeof(pid_t) * pnum);
    num_children = pnum;

    if (!with_files) {
        for (int i = 0; i < pnum; i++)
            if (pipe(pipes[i]) == -1) { perror("Pipe failed"); free(array); free(child_pids); return 1; }
    }

    if (timeout > 0) {
        signal(SIGALRM, alarm_handler);
        alarm(timeout);
        printf("Timeout set to %d seconds\n", timeout);
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int chunk_size = array_size / pnum;

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            int start = i * chunk_size;
            int end = (i == pnum - 1) ? array_size : start + chunk_size;
            struct MinMax mm = GetMinMax(array, start, end);

            if (with_files) {
                char fn[64];
                sprintf(fn, "temp_%d.txt", i);
                FILE *f = fopen(fn, "w");
                if (f) { fprintf(f, "%d %d\n", mm.min, mm.max); fclose(f); }
            } else {
                close(pipes[i][0]);
                write(pipes[i][1], &mm.min, sizeof(int));
                write(pipes[i][1], &mm.max, sizeof(int));
                close(pipes[i][1]);
            }
            free(array);
            return 0;
        } else if (child_pid < 0) {
            printf("Fork failed!\n");
            free(array);
            free(child_pids);
            return 1;
        } else {
            child_pids[i] = child_pid;
        }
    }

    if (!with_files) {
        for (int i = 0; i < pnum; i++) close(pipes[i][1]);
    }

    for (int i = 0; i < pnum; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (WIFSIGNALED(status))
            printf("Child %d was killed by signal %d\n", child_pids[i], WTERMSIG(status));
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX, max = INT_MIN;
        if (with_files) {
            char fn[64];
            sprintf(fn, "temp_%d.txt", i);
            FILE *f = fopen(fn, "r");
            if (f) { fscanf(f, "%d %d", &min, &max); fclose(f); remove(fn); }
        } else {
            read(pipes[i][0], &min, sizeof(int));
            read(pipes[i][0], &max, sizeof(int));
            close(pipes[i][0]);
        }
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed);
    fflush(NULL);

    return 0;
}