#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "encoding.h"


#define ARRAY_SIZE 1000

#define MINUTES_TO_RUN 5
#define SECS_PER_MIN 60
#define SECONDS_TO_RUN (MINUTES_TO_RUN * SECS_PER_MIN)
#define MAX_FREQUENCY (1000LL * 1000LL)
#define CYCLES_TO_RUN (MAX_FREQUENCY * SECONDS_TO_RUN)

void do_work() {
  int *arr = (int*)malloc(sizeof(int) * ARRAY_SIZE);
  for (int i = 0; i < ARRAY_SIZE; i++) {
    arr[i] = i * i + i;
  }
  int sum = 0;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    sum += arr[i];
  }
  free(arr);
}

uint64_t measure_cycles() {
  uint64_t start = rdcycle();
  do_work();
  uint64_t end = rdcycle();
  return (end - start);
}

void run_for_cycles(uint64_t min_cycles) {
  uint64_t total_cycles = 0;
  do {
    total_cycles += measure_cycles();
  } while (total_cycles < min_cycles);
  printf("Test ran for %" PRIu64 " cycles\n", total_cycles);
}

int main() {
  run_for_cycles(CYCLES_TO_RUN);
  return 0;
}
