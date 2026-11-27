#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Bảng trang do kernel in thẳng ra console, chương trình không đọc lại được,
// nên hai kiểm tra one-shot và không kế thừa qua fork (V8, V8b) phải xem output.

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
    exec("echo", echo_child);  // cờ không được kế thừa qua fork
    exit(1);
  }
  wait(0);

  exec("echo", echo_self);  // cờ đã bị xóa ở lần exec trước
  exit(1);
}

int
main(int argc, char *argv[])
{
  int fails = 0;
  char *args[] = { "vmprinttest", "stage2", 0 };

  if(argc >= 2 && strcmp(argv[1], "stage2") == 0)
    stage2();

  // setvmprint() luôn trả về 0
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
  printf("vmprinttest: setvmprint checks passed; V8/V8b below need a manual check\n");

  setvmprint(1);
  exec("vmprinttest", args);
  printf("[FAIL] exec vmprinttest\n");
  exit(1);
}
