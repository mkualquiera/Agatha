#if INTERFACE

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#endif

#include "benchmarking.h"

unsigned long benchmarking_read_off_memory_status()
{
  unsigned long dummy;
  unsigned long result;
  const char* statm_path = "/proc/self/statm";

  FILE *f = fopen(statm_path,"r");
  if(!f){
    perror(statm_path);
    abort();
  }
  if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
    &dummy,&result,&dummy,&dummy,&dummy,&dummy,&dummy))
  {
    perror(statm_path);
    abort();
  }
  fclose(f);
  return result * 4;
}

FILE *benchmark_file;
struct timeval start, end;
signed long startmemory, endmemory;
char *procedure = "";
long id;

void benchmarking_init() {
  id = 0;
  benchmark_file = fopen("benchmark", "w");
}

void benchmarking_probe_a(char *procedure_in) {
  procedure = procedure_in;
  startmemory = benchmarking_read_off_memory_status();
  gettimeofday(&start, NULL);
}

void benchmarking_probe_b() {
  id++;
  gettimeofday(&end, NULL);
  endmemory = benchmarking_read_off_memory_status();
  double time_taken;
  time_taken = (end.tv_sec - start.tv_sec) * 1e6;
  time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
  signed long mem_delta = endmemory - startmemory;
  fprintf(benchmark_file, "%s %lu %ld %ld %ld %f\n", procedure, id, startmemory, endmemory, mem_delta, time_taken);
}

void benchmarking_end() {
  fclose(benchmark_file);
}
