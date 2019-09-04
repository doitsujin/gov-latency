#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ONE_SEC  1000000000ul
#define ONE_MSEC 1000000ul

typedef struct timespec timespec_t;

typedef struct entry {
  uint64_t time_nsec;
  uint64_t freq_hz;
} entry_t;

typedef struct entry_buffer {
  size_t entry_count;
  size_t entry_index;
  entry_t* entry_array;
} entry_buffer_t;


void entry_buffer_create(entry_buffer_t* buffer, size_t entry_count) {
  buffer->entry_count = entry_count;
  buffer->entry_index = 0;
  buffer->entry_array = malloc(sizeof(entry_t) * entry_count);
}


void entry_buffer_destroy(entry_buffer_t* buffer) {
  free(buffer->entry_array);
  buffer->entry_count = 0;
  buffer->entry_index = 0;
  buffer->entry_array = NULL;
}


uint32_t entry_buffer_add(entry_buffer_t* buffer, entry_t entry) {
  if (buffer->entry_index < buffer->entry_count) {
    buffer->entry_array[buffer->entry_index++] = entry;
    return 1;
  } return 0;
}


void entry_buffer_print(const entry_buffer_t* buffer) {
  for (size_t i = 0; i < buffer->entry_index; i++) {
    printf("%f %f\n",
      (double)(buffer->entry_array[i].time_nsec) / (double)(ONE_MSEC),
      (double)(buffer->entry_array[i].freq_hz)   / (double)(ONE_MSEC));
  }
}


timespec_t time_get() {
  timespec_t result;
  clock_gettime(CLOCK_MONOTONIC_RAW, &result);
  return result;
}


uint64_t time_delta(timespec_t a, timespec_t b) {
  uint64_t result = 0;
  result += a.tv_sec;
  result -= b.tv_sec;
  result *= ONE_SEC;
  result += a.tv_nsec;
  result -= b.tv_nsec;
  return result;
}


uint64_t cpu_freq(uint64_t delta, uint64_t loop_iterations) {
  return (loop_iterations * ONE_SEC) / delta;
}


uint64_t parse_number(char* number) {
  uint64_t result = 0;
  for (uint64_t i = 0; number[i] != '\0'; i++) {
    result *= 10;
    result += number[i] - '0';
  }
  return result;
}


void execute_loop(uint64_t loop_iterations) {
  // 2 cycles per iteration on most CPUs
  asm volatile (
    "mov  %0, %%rcx;"
    ".align 32; 1:"
    "sub  $1, %%rcx;"
    "sub  $1, %%rcx;"
    "jnz 1b;"
    :
    : "r" (loop_iterations)
    : "rcx"
  );
}


int main(int argc, char** argv) {
  // The number of measurements to make
  // can be passed as an argument
  const size_t entry_count_default = 2000;
  const size_t entry_count = argc >= 2
    ? parse_number(argv[1])
    : entry_count_default;
  
  entry_buffer_t entry_buffer;
  entry_buffer_create(&entry_buffer, entry_count);
  
  // Same goes for the loop iteration count
  const uint64_t loop_iterations_default = 500000ul;
  const uint64_t loop_iterations = argc >= 3
    ? parse_number(argv[2])
    : loop_iterations_default;
  
  // Sleep for a second to let the CPU calm down
  // (unless it is loaded with background tasks)
  timespec_t sleep_time;
  sleep_time.tv_sec  = 1;
  sleep_time.tv_nsec = 0;
  nanosleep(&sleep_time, NULL);
  
  // Now start measuring the frequencies while at
  // the same time stressing the CPU. The governor
  // shall take action from now on.
  timespec_t prev_time   = time_get();
  uint64_t   time_offset = 0;
  uint32_t   do_continue = 1;
  
  while (do_continue) {
    execute_loop(loop_iterations);
    
    const timespec_t curr_time = time_get();
    const uint64_t delta = time_delta(curr_time, prev_time);
    
    entry_t entry;
    entry.time_nsec = time_offset;
    entry.freq_hz   = cpu_freq(delta, loop_iterations);
    do_continue     = entry_buffer_add(&entry_buffer, entry);
    
    time_offset += delta;
    prev_time    = curr_time;
  }
  
  // Done, we can print the results to the
  // console. The format is gnuplot friendly.
  entry_buffer_print  (&entry_buffer);
  entry_buffer_destroy(&entry_buffer);
  return 0;
}
