#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "usage: pgtbl program [args...]\n");
    exit(1);
  }

  // ask the kernel to print the page table on the next successful exec
  setvmprint(1);
  exec(argv[1], &argv[1]);

  fprintf(2, "pgtbl: exec %s failed\n", argv[1]);
  exit(1);
}
