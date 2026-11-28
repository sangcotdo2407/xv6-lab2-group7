// Chương trình kiểm thử trường freemem mà sysinfo() trả thêm ở Lab 2.
// Mã test F1..F7 theo TASK_ASSIGNMENT.md mục 4.

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

#define TEST_PGSIZE 4096ULL
#define TEST_PHYS_LIMIT (128ULL * 1024 * 1024)
#define TEST_ALLOC_PAGES 32

// struct sysinfo khoảng 38 KB, lớn hơn stack một trang của tiến trình user,
// nên phải đặt static.
static struct sysinfo si;
static int failed;
// Khai báo trước để F7 dùng địa chỉ của main, một địa chỉ trong trang text.
int main(void);

static void
check(int condition, char *message)
{
  if(condition)
    printf("[PASS] %s\n", message);
  else {
    printf("[FAIL] %s\n", message);
    failed++;
  }
}

static int
snapshot(void)
{
  return sysinfo(&si);
}

static int
has_process(int pid)
{
  if(si.process_count < 0 || si.process_count > NPROC)
    return 0;

  for(int i = 0; i < si.process_count; i++)
    if(si.processes[i].pid == pid)
      return 1;

  return 0;
}

static void
test_basic(void)
{
  int ret = snapshot();

  check(ret == 0, "F1: sysinfo tra ve thanh cong");
  if(ret < 0)
    return;

  check(si.freemem > 0, "F1: freemem lon hon 0");
  // RAM của xv6 là KERNBASE..PHYSTOP (128 MiB), một phần đã thuộc kernel.
  check(si.freemem < TEST_PHYS_LIMIT, "F1: freemem nho hon 128 MiB");
  check(si.freemem % TEST_PGSIZE == 0, "F1: freemem chia het cho PGSIZE");
}

static void
test_repeat(void)
{
  uint64 first;

  if(snapshot() < 0) {
    check(0, "F2: hai snapshot lien tiep on dinh");
    return;
  }
  first = si.freemem;

  // Giữa hai lần gọi không có cấp phát nào (shell đang chờ wait()),
  // nên freemem phải bằng nhau.
  check(snapshot() == 0 && si.freemem == first,
        "F2: hai lan goi lien tiep khong cap phat cho cung freemem");
}

static void
test_sbrk(void)
{
  int amount = TEST_ALLOC_PAGES * (int)TEST_PGSIZE;
  uint64 before;
  uint64 during;
  uint64 after;
  char *area;

  if(snapshot() < 0) {
    check(0, "F3: sbrk(+N trang) giam freemem theo so trang");
    check(0, "F4: sbrk(-N trang) tang freemem dung N trang");
    return;
  }
  before = si.freemem;

  area = sbrk(amount);
  if(area == (char *)-1) {
    check(0, "F3: sbrk(+N trang) cap phat thanh cong");
    check(0, "F4: sbrk(-N trang) giai phong dung N trang");
    return;
  }

  if(snapshot() < 0) {
    sbrk(-amount);
    check(0, "F3: doc freemem sau khi cap phat");
    check(0, "F4: doc freemem sau khi giai phong");
    return;
  }
  during = si.freemem;

  // Ngoài N trang dữ liệu, uvmalloc() có thể cấp thêm page-table page khi
  // vùng mới cần một bảng trang cấp dưới chưa có; N+3 là cận trên có dư.
  check(before >= during &&
        before - during >= (uint64)TEST_ALLOC_PAGES * TEST_PGSIZE &&
        before - during <= (uint64)(TEST_ALLOC_PAGES + 3) * TEST_PGSIZE,
        "F3: cap phat N trang giam tu N den N+3 trang");

  if(sbrk(-amount) == (char *)-1) {
    check(0, "F4: sbrk(-N trang) thuc hien thanh cong");
    return;
  }

  if(snapshot() < 0) {
    check(0, "F4: doc freemem sau khi giai phong");
    return;
  }
  after = si.freemem;

  // sbrk(-N) chỉ trả lại các trang lá; page-table page được giữ đến khi
  // tiến trình thoát, nên freemem tăng đúng N trang so với lúc sau cấp phát.
  check(after >= during && after - during == (uint64)amount,
        "F4: giai phong N trang tang dung N*PGSIZE byte");
}

static void
test_fork(void)
{
  int p[2];
  int before_ok;
  int during_ok;
  int after_ok;
  int pid;
  int status = -1;
  int waited;
  int signaled;
  uint64 before = 0;
  uint64 during = 0;
  uint64 after = 0;
  char token = 'x';

  // Pipe được tạo trước lần đo đầu nên trang của pipe không làm lệch kết quả.
  // Con chặn ở read() để còn sống trong lúc cha đo, rồi thoát khi cha ghi.
  if(pipe(p) < 0) {
    check(0, "F5: fork lam giam freemem khi child con song");
    check(0, "F5: freemem tro lai muc truoc fork sau wait");
    return;
  }

  before_ok = snapshot() == 0;
  if(before_ok)
    before = si.freemem;

  pid = fork();
  if(pid < 0) {
    check(0, "F5: fork lam giam freemem khi child con song");
    check(0, "F5: freemem tro lai muc truoc fork sau wait");
    close(p[0]);
    close(p[1]);
    return;
  }

  if(pid == 0) {
    char received;
    if(read(p[0], &received, 1) != 1)
      exit(1);
    exit(0);
  }

  during_ok = snapshot() == 0;
  if(during_ok)
    during = si.freemem;

  signaled = write(p[1], &token, 1) == 1;
  if(!signaled)
    kill(pid);
  waited = wait(&status);

  after_ok = snapshot() == 0;
  if(after_ok)
    after = si.freemem;

  // Sau wait(), bộ nhớ user, page table và trapframe của con đã được trả
  // lại, nên freemem phải đúng bằng giá trị trước fork.
  check(before_ok && during_ok && during < before,
        "F5: fork lam giam freemem khi child con song");
  check(signaled && waited == pid && status == 0,
        "F5: child nhan tin hieu va duoc wait thanh cong");
  check(before_ok && after_ok && after == before,
        "F5: freemem tro lai muc truoc fork sau wait");

  close(p[0]);
  close(p[1]);
}

static void
test_process_fields(void)
{
  int ret = snapshot();

  check(ret == 0, "F6: sysinfo van doc duoc cac truong Lab 1");
  if(ret < 0)
    return;

  check(si.process_count >= 2, "F6: process_count van hop le");
  check(has_process(getpid()), "F6: tim thay PID cua tien trinh hien tai");
}

static void
test_bad_pointers(void)
{
  // Địa chỉ 0 và main nằm trong trang text không có PTE_W nên copyout() từ
  // chối; 1 << 38 chính là MAXVA.
  check(sysinfo((struct sysinfo *)0) < 0,
        "F7: con tro NULL bi tu choi");
  check(sysinfo((struct sysinfo *)(uint64)main) < 0,
        "F7: dia chi vung text chi doc bi tu choi");
  check(sysinfo((struct sysinfo *)(1ULL << 38)) < 0,
        "F7: dia chi tu MAXVA tro len bi tu choi");
}

int
main(void)
{
  printf("===== freememtest =====\n");

  test_basic();
  test_repeat();
  test_sbrk();
  test_fork();
  test_process_fields();
  test_bad_pointers();

  if(failed == 0)
    printf("freememtest: ALL PASSED\n");
  else
    printf("freememtest: %d FAILED\n", failed);

  exit(failed != 0);
}
