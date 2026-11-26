#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// The page table is printed by the kernel straight to the console, so the
// one-shot / no-inherit checks (V8, V8b) are verified by reading the output.

static void
stage2(void)
{
  int pid;
  char *echo_child[] = { "echo", "child exec: no page table expected", 0 };
  char *echo_self[] = { "echo", "stage2 exec: no page table expected", 0 };

  printf("stage2: exactly one 'page table' block must appear above this line\n");

  pid = fork();
  if(pid < 0){
    printf("[FAIL] fork\n");
    exit(1);
  }
  if(pid == 0){
    exec("echo", echo_child);  // flag is not inherited through fork
    exit(1);
  }
  wait(0);

  exec("echo", echo_self);  // flag was cleared by the previous exec
  exit(1);
}

int
main(int argc, char *argv[])
{
  int fails = 0;
  char *args[] = { "vmprinttest", "stage2", 0 };

  if(argc >= 2 && strcmp(argv[1], "stage2") == 0)
    stage2();

  // setvmprint() always returns 0
  if(setvmprint(1) == 0 && setvmprint(0) == 0 && setvmprint(-5) == 0 &&
     setvmprint(0) == 0)
    printf("[PASS] setvmprint return values\n");
  else {
    printf("[FAIL] setvmprint return values\n");
    fails++;
  }

  if(fails){
    printf("vmprinttest: %d FAILED\n", fails);
    exit(1);
  }
  printf("vmprinttest: ALL PASSED (return values); check the one-shot output below\n");

  setvmprint(1);
  exec("vmprinttest", args);
  printf("[FAIL] exec vmprinttest\n");
  exit(1);
}
