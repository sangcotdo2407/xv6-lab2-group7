// Chương trình kiểm thử system call pgaccess().
// Mã test (P1..P13) và các pha (A..D) theo TASK_ASSIGNMENT.md mục 6.

#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

#define NPAGES 64  // kích thước vùng test, bằng giới hạn của kernel

static char *region;  // NPAGES trang mới cấp từ sbrk(), đã căn trang
static int failed;

static void
check(int ok, char *what)
{
  if(ok){
    printf("[PASS] %s\n", what);
  } else {
    printf("[FAIL] %s\n", what);
    failed++;
  }
}

static void
check_mask(int ret, uint64 got, uint64 want, char *what)
{
  check(ret == 0 && got == want, what);
  if(ret != 0 || got != want)
    printf("       got ret %d mask %p, want ret 0 mask %p\n", ret, got, want);
}

static uint64
bit(int i)
{
  return 1UL << i;
}

// Truy cập qua con trỏ volatile để compiler không bỏ
// hay gộp mất các lần truy cập.
static void
read_page(int i)
{
  volatile char *p = region + (uint64)i * PGSIZE;
  (void)*p;
}

static void
write_page(int i)
{
  volatile char *p = region + (uint64)i * PGSIZE;
  *p = (char)i;
}

// Mask được đặt về 0 trước vì kernel chỉ ghi (len+7)/8 byte.
static int
scan_from(char *base, int len, uint64 *mask)
{
  *mask = 0;
  return pgaccess(base, len, mask);
}

// Gọi pgaccess() trên len trang đầu của vùng. Mask nằm trên stack,
// ngoài vùng, nên việc ghi mask không chạm trang nào trong vùng.
static int
scan(int len, uint64 *mask)
{
  return scan_from(region, len, mask);
}

// Bốn pha A-D của quy trình kiểm chứng bit PTE_A.
static void
test_phases(void)
{
  uint64 m;
  int r;

  r = scan(32, &m);
  check_mask(r, m, 0, "P1/A: fresh sbrk pages are not accessed");

  read_page(1);
  write_page(2);
  write_page(30);
  r = scan(32, &m);
  check_mask(r, m, bit(1) | bit(2) | bit(30),
             "P1/B: read page 1, wrote pages 2 and 30");

  r = scan(32, &m);
  check_mask(r, m, 0, "P2/C: second call reports nothing");

  write_page(2);
  r = scan(32, &m);
  check_mask(r, m, bit(2), "P12/D: page 2 reported again after re-touch");
}

static void
test_read_write(void)
{
  uint64 m;
  int r;

  scan(NPAGES, &m);
  read_page(5);
  r = scan(NPAGES, &m);
  check_mask(r, m, bit(5), "P3: a read alone is reported");

  write_page(6);
  r = scan(NPAGES, &m);
  check_mask(r, m, bit(6), "P3: a write alone is reported");
}

static void
test_len(void)
{
  uint64 m;
  int r;

  scan(NPAGES, &m);
  write_page(0);
  write_page(63);
  r = scan(64, &m);
  check_mask(r, m, bit(0) | bit(63), "P4: len = 64 reports bits 0 and 63");

  check(pgaccess(region, 65, &m) == -1, "P4: len = 65 rejected");
  check(pgaccess(region, 0, &m) == -1, "P4: len = 0 rejected");
  check(pgaccess(region, -1, &m) == -1, "P4: len = -1 rejected");
}

// base lệch trang được làm tròn xuống trang chứa nó.
static void
test_base(void)
{
  uint64 m;
  int r;

  scan(NPAGES, &m);
  write_page(1);
  r = scan_from(region + 1, 2, &m);
  check_mask(r, m, bit(1), "P5: base = region + 1 starts at page 0");

  write_page(1);
  write_page(4);
  r = scan_from(region + PGSIZE + PGSIZE / 2, 4, &m);
  check_mask(r, m, bit(0) | bit(3),
             "P5: base = region + 1.5 pages starts at page 1");

  write_page(NPAGES - 1);
  r = scan_from(region + NPAGES * PGSIZE - 1, 1, &m);
  check_mask(r, m, bit(0), "P5: base = last byte of the region maps to page 63");

  check(pgaccess(region + NPAGES * PGSIZE, 1, &m) == -1,
        "P6: page at sbrk(0) rejected");
  check(pgaccess(region + (NPAGES - 1) * PGSIZE, 2, &m) == -1,
        "P6: range crossing sbrk(0) rejected");
  check(pgaccess(region + NPAGES * PGSIZE - 1, 2, &m) == -1,
        "P6: unaligned base whose range crosses sbrk(0) rejected");
  check(pgaccess((void *)0xffffffffffffffffUL, 1, &m) == -1,
        "P6: base = 0xffffffffffffffff rejected");
  check(pgaccess((void *)MAXVA, 1, &m) == -1, "P6: base = MAXVA rejected");
  check(pgaccess((void *)0xffffffffffff0000UL, 1, &m) == -1,
        "P6: base = 0xffffffffffff0000 rejected");
  check(pgaccess((void *)(0UL - PGSIZE), 64, &m) == -1,
        "P6: base + len*PGSIZE wraps around, rejected");
}

// Mask không hợp lệ phải trả lỗi mà không xóa PTE_A (P7, rồi P11).
static void
test_bad_mask(void)
{
  uint64 m;
  int r;

  scan(NPAGES, &m);
  write_page(10);
  read_page(11);

  check(pgaccess(region, NPAGES, 0) == -1, "P7: mask = 0 rejected");
  check(pgaccess(region, NPAGES, (void *)test_bad_mask) == -1,
        "P7: mask in read-only text rejected");
  check(pgaccess(region, NPAGES, region + NPAGES * PGSIZE) == -1,
        "P7: unmapped mask rejected");
  check(pgaccess(region, NPAGES, (void *)MAXVA) == -1,
        "P7: mask = MAXVA rejected");

  r = scan(NPAGES, &m);
  check_mask(r, m, bit(10) | bit(11),
             "P11: failed calls kept the earlier accesses");
}

// Mask nằm trong chính vùng đang kiểm tra: kernel ghi mask qua direct map
// nên không bật PTE_A của trang chứa mask.
static void
test_mask_in_region(void)
{
  volatile uchar *inside = (uchar *)region + 3 * PGSIZE;
  uint64 m;
  int r, r2;

  scan(NPAGES, &m);
  write_page(1);
  r = pgaccess(region, 4, (void *)inside);
  r2 = scan(4, &m);
  check(r == 0 && r2 == 0 && m == 0,
        "P13: writing the mask into page 3 does not set its A bit");
  check(*inside == 0x02, "P13: mask stored inside the region is correct");
}

// Kernel phải ghi đúng (len+7)/8 byte của mask.
static void
test_partial_byte(void)
{
  uchar out[4];
  uint64 m;
  int r;

  scan(NPAGES, &m);
  write_page(0);
  write_page(8);
  memset(out, 0xAA, sizeof(out));
  r = pgaccess(region, 9, out);
  check(r == 0 && out[0] == 0x01 && out[1] == 0x01 &&
        out[2] == 0xAA && out[3] == 0xAA,
        "P8: len = 9 writes exactly 2 bytes, canary intact");

  memset(out, 0xAA, sizeof(out));
  r = pgaccess(region, 1, out);
  check(r == 0 && out[0] == 0x00 && out[1] == 0xAA,
        "P8: len = 1 writes exactly 1 byte, canary intact");
}

// exec() để lại một guard page không có PTE_U ngay dưới trang stack.
static void
test_guard(void)
{
  int local;
  uint64 stack = PGROUNDDOWN((uint64)&local);
  uint64 m;
  int r;

  m = 0;  // với len = 2, kernel chỉ ghi byte thấp
  r = pgaccess((void *)(stack - PGSIZE), 2, &m);
  check_mask(r, m, bit(1), "P9: guard page reads 0, stack page reads 1");
}

// fork() sao chép cờ của PTE, nên truy cập trước fork hiện ở cả hai
// tiến trình; truy cập sau fork là riêng của từng bên.
static void
test_fork(void)
{
  uint64 m;
  int r, pid, status;

  scan(NPAGES, &m);
  write_page(7);

  pid = fork();
  if(pid < 0){
    check(0, "P10: fork");
    return;
  }
  if(pid == 0){
    write_page(3);
    r = scan(NPAGES, &m);
    if(r != 0 || m != (bit(3) | bit(7))){
      printf("       child: got ret %d mask %p\n", r, m);
      exit(1);
    }
    exit(0);
  }

  write_page(4);
  status = -1;
  if(wait(&status) != pid)
    status = -1;
  check(status == 0, "P10: child sees its page 3 and inherited page 7");
  r = scan(NPAGES, &m);
  check_mask(r, m, bit(4) | bit(7),
             "P10: parent sees pages 4 and 7, not the child's page 3");
}

// Xóa, sleep (tiến trình có thể chạy tiếp trên CPU khác), chạm trang, kiểm tra.
static void
test_migrate(void)
{
  uint64 m;
  int i, pg, r, bad = 0;

  scan(NPAGES, &m);
  for(i = 0; i < 20; i++){
    pg = (i * 7) % NPAGES;
    sleep(1);
    write_page(pg);
    r = scan(NPAGES, &m);
    if(r != 0 || m != bit(pg)){
      printf("       round %d: got ret %d mask %p, want %p\n", i, r, m, bit(pg));
      bad++;
    }
  }
  check(bad == 0, "P12: 20 clear/sleep/touch rounds report exactly one page");
}

// Cách dùng như pgaccess_test() mà đề gợi ý: buffer lấy từ malloc nên lệch
// 16 byte so với đầu trang. Chạy cuối cùng vì malloc làm sbrk(0) dời đi.
static void
test_malloc(void)
{
  volatile char *buf;
  uint64 m;
  int r;

  buf = malloc(32 * PGSIZE);
  if(buf == 0){
    check(0, "P5: malloc(32*PGSIZE)");
    return;
  }
  check((uint64)buf % PGSIZE != 0, "P5: malloc'd buffer is not page-aligned");
  scan_from((char *)buf, 32, &m);
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;
  r = scan_from((char *)buf, 32, &m);
  check_mask(r, m, bit(1) | bit(2) | bit(30),
             "P5: malloc'd buffer reports pages 1, 2, 30");
  free((void *)buf);
}

int
main(void)
{
  char *brk;

  // Căn break theo trang để vùng bắt đầu ở đầu trang
  // và kết thúc đúng tại sbrk(0).
  brk = sbrk(0);
  if(sbrk(PGROUNDUP((uint64)brk) - (uint64)brk) == (char *)-1 ||
     (region = sbrk(NPAGES * PGSIZE)) == (char *)-1){
    printf("pgtbltest: sbrk failed\n");
    exit(1);
  }

  test_phases();
  test_read_write();
  test_len();
  test_base();
  test_bad_mask();
  test_mask_in_region();
  test_partial_byte();
  test_guard();
  test_fork();
  test_migrate();
  test_malloc();

  if(failed == 0)
    printf("pgtbltest: ALL PASSED\n");
  else
    printf("pgtbltest: %d FAILED\n", failed);
  exit(failed != 0);
}
