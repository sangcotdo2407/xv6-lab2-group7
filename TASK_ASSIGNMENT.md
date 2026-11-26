# TASK_ASSIGNMENT.md — Phân công nhiệm vụ Lab 2 (Memory)

Nhóm 7 · Baseline: commit `cd1e597`, tag `lab2-baseline` · Quy tắc chung: [AGENTS.md](AGENTS.md)

Ký hiệu nguồn gốc của mỗi quyết định:

- **[Đề]**: yêu cầu bắt buộc trong `Lab2_Memory.pdf`.
- **[Quy ước nhóm]**: quyết định kỹ thuật đề xuất của nhóm để làm song song không xung đột. Không phải yêu cầu của giảng viên; muốn đổi phải thống nhất cả nhóm và cập nhật tài liệu này trước khi code.

---

## 1. Tổng quan và quy trình

| Thành viên | MSSV | Nhiệm vụ | Điểm | Nhánh | Syscall mới |
|---|---|---|---|---|---|
| Mai Khắc Hoàng Vũ | 24120183 | Enhance Sysinfo system call | 2 | `feature/sysinfo-memory` | Không (mở rộng `SYS_sysinfo` = 22) |
| Hồ Sỹ Thi | 24120452 | Print a page table | 4 | `feature/vmprint` | `SYS_setvmprint` = 23 |
| Đặng Quang Sang | 24120428 | Detect which pages have been accessed; tích hợp và nộp bài | 4 | `feature/pgaccess` | `SYS_pgaccess` = 24 |

Quy trình Git:

1. Tài liệu này được review và push lên `main`. Mỗi thành viên `git switch main && git pull`.
2. Mỗi người tạo nhánh tính năng từ `main`: `git switch -c feature/<tên> main`.
3. Làm độc lập, chỉ trong phạm vi ở §3. Không cần chờ người khác vì mọi giao diện dùng chung đã chốt ở §2.
4. Tự build, kiểm thử (AGENTS.md §9), viết báo cáo cá nhân.
5. Push nhánh, tạo PR vào `integration/lab2` (Sang tạo nhánh này từ `main`).
6. Sang review, sửa, giải quyết conflict, merge theo thứ tự **Vũ → Thi → Sang**, chạy hồi quy sau mỗi lần merge.
7. Kiểm thử tổng thể, tổng hợp báo cáo, tạo patch và Source.zip (§7).

Không dùng Issues, GitHub Projects hay GitHub Actions.

---

## 2. Quy ước dùng chung (nguồn chuẩn duy nhất)

### 2.1. Số hiệu system call

Số đang dùng tại baseline: 1–22 (`SYS_fork` = 1 … `SYS_close` = 21, `SYS_sysinfo` = 22). Số mới được cấp tuần tự từ 23.

| Số | Macro (`kernel/syscall.h`) | Handler kernel | API phía user (`user/user.h`) | Chủ sở hữu | Nguồn |
|---|---|---|---|---|---|
| 22 | `SYS_sysinfo` (giữ nguyên) | `sys_sysinfo` (mở rộng) | `int sysinfo(struct sysinfo *);` (giữ nguyên) | Vũ | Lab 1 |
| 23 | `SYS_setvmprint` | `uint64 sys_setvmprint(void)` | `int setvmprint(int on);` | Thi | [Quy ước nhóm] |
| 24 | `SYS_pgaccess` | `uint64 sys_pgaccess(void)` | `int pgaccess(void *base, int len, void *mask);` | Sang | Tên [Đề]; chữ ký [Quy ước nhóm] |

Không ai được dùng số 23/24 cho mục đích khác. Nếu cần thêm syscall ngoài bảng này, phải cập nhật bảng trước.

### 2.2. Giao diện kernel mới

| Khai báo | File định nghĩa | Khai báo ở | Chủ sở hữu | Nguồn |
|---|---|---|---|---|
| `uint64 kfreebytes(void);` — trả về số byte bộ nhớ vật lý còn trống trong allocator | `kernel/kalloc.c` (cuối file) | `kernel/defs.h`, mục `// kalloc.c`, ngay sau `void kinit(void);` | Vũ | [Quy ước nhóm] |
| `void vmprint(pagetable_t);` | `kernel/vm.c` (cuối file) | `kernel/defs.h`, mục `// vm.c`, ngay sau dòng `copyinstr` | Thi | [Đề] |
| `#define PTE_A (1L << 6)` — accessed bit (bit 6 của PTE Sv39 theo RISC-V Privileged Spec) | `kernel/riscv.h`, ngay sau `PTE_U` | — | Sang | [Đề] yêu cầu định nghĩa; giá trị theo spec |
| Trường `int vmprint_on_exec;` trong `struct proc` | `kernel/proc.h`, nhóm "private to the process", sau `char name[16];` | — | Thi | [Quy ước nhóm] |
| `#define PGACCESS_MAXPAGES 64` (cục bộ) | `kernel/sysproc.c`, ngay trên `sys_pgaccess` | — | Sang | [Quy ước nhóm] |

Chỉ Sang định nghĩa `PTE_A`. Thi không cần `PTE_A` để cài `vmprint`; khi giải thích bit trong báo cáo, Thi tự giải mã theo Hình 3.2 sách.

### 2.3. Mở rộng `struct sysinfo` (ABI dùng chung kernel ↔ user) — [Quy ước nhóm]

- Thêm **đúng một** trường `uint64 freemem;` (đơn vị: byte) vào **cuối** `struct sysinfo` trong `kernel/sysinfo.h`, không kèm comment (AGENTS.md §7.1).
- Không đổi thứ tự, kiểu hay tên trường cũ: offset của mọi trường Lab 1 giữ nguyên.
- `freemem` được điền trong `sys_sysinfo()`, **sau khi** `collect_file_info()` và `collect_process_info()` thành công, **trước** `copyout()`, khi vẫn giữ `kinfo_lock`.
- Không sửa hai collector của Lab 1. Khi collector lỗi: trả `-1` và **không** `copyout` (giữ đúng ngữ nghĩa mà `tests/sysinfo_fault.c` kiểm tra).
- Kích thước struct tăng 8 byte: mọi chương trình include `kernel/sysinfo.h` phải build lại (`make clean`).

### 2.4. Cơ chế in bảng trang theo yêu cầu — [Đề] + [Quy ước nhóm]

- [Đề] `vmprint()` đặt trong `vm.c`, prototype trong `defs.h`, được gọi trong `exec.c` ngay trước `return argc;`.
- [Quy ước nhóm] Lời gọi đó **có điều kiện**: chỉ in khi `p->vmprint_on_exec != 0`. Khi cờ bằng 0 (mặc định), `exec` chạy y hệt baseline, kể cả `/init` lúc khởi động.
- [Quy ước nhóm] Cờ được bật bởi syscall `setvmprint(on)`:
  - `on != 0` → bật cho lần `exec` thành công **kế tiếp** của chính tiến trình gọi; `on == 0` → tắt. Luôn trả `0`.
  - Cờ là **một lần (one-shot)**: `exec` xóa cờ khi in. Đường lỗi (`bad:`) không in.
  - Cờ **không** được kế thừa qua `fork`: `freeproc()` đặt lại `0`, nên `allocproc()` luôn nhận slot có cờ bằng 0.
  - Cờ chỉ do chính tiến trình đọc/ghi (giống `p->sz`), không cần `p->lock`.
- [Quy ước nhóm] Chương trình người dùng `pgtbl`: `pgtbl <program> [args...]` gọi `setvmprint(1)` rồi `exec(program, args)`. Nếu `exec` lỗi thì in lỗi ra fd 2 và `exit(1)`; nếu thiếu đối số thì in hướng dẫn sử dụng và `exit(1)`.
- Không đổi chữ ký `exec()`, không sửa `sys_exec()`.

Định dạng output [Đề] (mỗi mức sâu thêm một cụm ` ..`, giá trị in bằng `%p` = `0x` + 16 chữ số hex):

```text
page table 0x0000000087f6b000
 ..0: pte 0x0000000021fd9c01 pa 0x0000000087f67000
 .. ..0: pte 0x0000000021fd9801 pa 0x0000000087f66000
 .. .. ..0: pte 0x0000000021fda01b pa 0x0000000087f68000
```

### 2.5. Ngữ nghĩa `pgaccess()` — [Đề] + [Quy ước nhóm]

`int pgaccess(void *base, int len, void *mask);` trả `0` nếu thành công, `-1` nếu lỗi.

| Quy tắc | Nguồn |
|---|---|
| Bit `i` (0 ≤ `i` < `len`) ứng với trang thứ `i` tính từ trang chứa `base`; trang đầu là bit thấp nhất | [Đề]; cách xác định trang đầu là [Quy ước nhóm] |
| Bit `i` nằm ở byte `i/8`, vị trí `i%8` của buffer `mask`; kernel ghi **đúng** `(len+7)/8` byte. Trên RISC-V little-endian, user có thể truyền `unsigned int*` (len ≤ 32) hoặc `uint64*` (len ≤ 64) | [Quy ước nhóm] |
| `base` không cần thẳng trang: kernel làm tròn xuống bằng `PGROUNDDOWN`. Lý do: `pgaccess_test()` trong `user/pgtbltest.c` mà [Đề] gợi ý tham khảo truyền buffer lấy từ `malloc` (lệch 16 byte so với đầu trang) | [Quy ước nhóm], đổi ngày 26/11/2026 (§8) |
| `1 ≤ len ≤ PGACCESS_MAXPAGES` (= 64), nếu không → `-1` | Đề cho phép đặt giới hạn; giá trị là [Quy ước nhóm] |
| Toàn bộ các trang được kiểm tra, `[PGROUNDDOWN(base), PGROUNDDOWN(base) + len*PGSIZE)`, phải nằm trong `[0, p->sz)` và dưới `MAXVA` (kiểm tra **trước** khi gọi `walk`), nếu không → `-1` | [Quy ước nhóm] |
| Trang được báo "đã truy cập" khi PTE hợp lệ, có `PTE_U` và có `PTE_A` | [Quy ước nhóm] |
| Trang trong vùng nhưng không có PTE user hợp lệ (ví dụ guard page dưới stack) → bit 0, không lỗi | [Quy ước nhóm] |
| Sau khi báo cáo, xóa `PTE_A` của các trang đã báo để lần gọi sau chỉ phản ánh truy cập mới. Không chạm `PTE_D` hay bit khác | [Đề] yêu cầu kiểm soát set/clear; chi tiết [Quy ước nhóm] |
| Khuyến nghị thứ tự: quét và lập mask (chỉ đọc) → `copyout` → chỉ khi `copyout` thành công mới xóa `PTE_A`. Nhờ vậy, khi `mask` không hợp lệ thì trả `-1` mà không mất thông tin truy cập | [Quy ước nhóm] |
| `mask` không hợp lệ (NULL, chưa map, chỉ đọc) → `-1` | [Quy ước nhóm] |

### 2.6. Tên chương trình kiểm thử và thứ tự trong `Makefile`

| File | Lệnh trong xv6 | Chủ sở hữu |
|---|---|---|
| `user/freememtest.c` | `freememtest` | Vũ |
| `user/pgtbl.c` | `pgtbl` (công cụ yêu cầu in bảng trang) | Thi |
| `user/vmprinttest.c` | `vmprinttest` (tùy chọn) | Thi |
| `user/pgtbltest.c` | `pgtbltest` (file mới, repo chưa có) | Sang |

- Thêm vào `UPROGS` **ngay sau** dòng `$U/_sysinfotest\`, mỗi dòng thụt bằng **tab**, theo thứ tự: `_freememtest`, `_pgtbl`, `_vmprinttest`, `_pgtbltest`.
- Không sửa dòng `$U/_sysinfotest\`: `tests/run_lab1_extra.py` yêu cầu dòng này xuất hiện đúng một lần với nội dung chính xác.
- Quy ước output: mỗi kiểm tra in `[PASS] <mô tả>` hoặc `[FAIL] <mô tả>`; dòng cuối là `<tên>: ALL PASSED` hoặc `<tên>: <n> FAILED`; `exit(0)` khi đạt và `exit(1)` khi có lỗi.

### 2.7. File báo cáo cá nhân

| Thành viên | File |
|---|---|
| Vũ | `docs/reports/24120183_sysinfo.md` |
| Thi | `docs/reports/24120452_vmprint.md` |
| Sang | `docs/reports/24120428_pgaccess.md` |

Ảnh minh chứng đặt trong `docs/reports/img/`, tên bắt đầu bằng MSSV (ví dụ `24120428_pgtbltest_cpu3.png`).

---

## 3. Ma trận file × thành viên và nguy cơ conflict

| File | Vũ | Thi | Sang | Conflict khi merge |
|---|---|---|---|---|
| `kernel/sysinfo.h` | Thêm `freemem` cuối `struct sysinfo` | — | — | Không |
| `kernel/kalloc.c` | Thêm `kfreebytes()` cuối file (nếu chọn bộ đếm thì sửa `kalloc`/`kfree`, xem §4) | — | — | Không |
| `kernel/defs.h` | 1 dòng trong mục `// kalloc.c` | 1 dòng trong mục `// vm.c` | — | Không (khác hunk) |
| `kernel/sysproc.c` | Chỉ sửa **thân** `sys_sysinfo` | Thêm `sys_setvmprint` ngay sau `sys_uptime`, trước comment `// The snapshot exceeds...` | Thêm `sys_pgaccess` ở **cuối file** | Không, nếu giữ đúng vị trí |
| `kernel/vm.c` | — | Thêm `vmprint` cuối file | — (dùng `walk` sẵn có) | Không |
| `kernel/exec.c` | — | Lời gọi có điều kiện ngay trước `return argc;` | — | Không |
| `kernel/proc.h` | — | Thêm trường `vmprint_on_exec` | — | Không |
| `kernel/proc.c` | — | 1 dòng đặt lại cờ trong `freeproc()` | — | Không |
| `kernel/riscv.h` | — | — | Thêm `PTE_A` sau `PTE_U` | Không |
| `kernel/syscall.h` | — | `SYS_setvmprint 23` | `SYS_pgaccess 24` | **Có**, dễ giải quyết |
| `kernel/syscall.c` | — | `extern` + phần tử bảng | `extern` + phần tử bảng | **Có**, dễ giải quyết |
| `user/usys.pl` | — | `entry("setvmprint");` | `entry("pgaccess");` | **Có**, dễ giải quyết |
| `user/user.h` | — | prototype `setvmprint` | prototype `pgaccess` | **Có**, dễ giải quyết |
| `Makefile` | `_freememtest` | `_pgtbl`, `_vmprinttest` | `_pgtbltest` | **Có**, dễ giải quyết |
| `user/<test>.c` | `freememtest.c` | `pgtbl.c`, `vmprinttest.c` | `pgtbltest.c` | Không (file mới) |
| `docs/reports/…` | file riêng | file riêng | file riêng | Không |

Cách giải quyết conflict ở các file "registry" (syscall.h/.c, usys.pl, user.h, Makefile): **giữ tất cả các dòng**. Syscall sắp theo số tăng dần (23 rồi 24); `UPROGS` theo thứ tự ở §2.6. Sau khi giải quyết, `make clean && make qemu` rồi chạy lại toàn bộ test.

Cấm sửa (cả ba người): `kernel/file.c`, `collect_process_info()` và `forkret()` trong `kernel/proc.c`, `sys_exec()` trong `kernel/sysfile.c`, `user/sysinfotest.c`, `user/filetest.c`, `user/usertests.c`, `user/forktest.c`, thư mục `tests/`, tag `lab2-baseline`.

Các harness Lab 1 trong `tests/*.py` tìm chuỗi trong mã nguồn để chèn code kiểm thử. Nếu các chuỗi dưới đây bị thay đổi, harness báo lỗi dù code vẫn đúng:

| Chuỗi phải giữ nguyên | Harness dùng |
|---|---|
| Dòng `\t$U/_sysinfotest\` xuất hiện **đúng một lần** trong `Makefile` | `run_lab1_extra.py` |
| `collect_file_info(struct sysinfo *info)` và `collect_process_info(struct sysinfo *info)` ngay trước dòng `{` | `run_lab1_extra.py` |
| `kernel/proc.c` có **đúng một** dòng `    __sync_synchronize();` và nó nằm trong `forkret()`, sau `fsinit(ROOTDEV);` và `first = 0;`. Không thêm `__sync_synchronize();` ở chỗ khác trong `proc.c` | `run_file_info.py`, `run_process_info.py` |

Ngoài ra `run_file_info.py` và `run_lab1_extra.py` gọi trực tiếp `riscv64-unknown-elf-gcc`/`readelf`, nên cần toolchain có tiền tố này trong WSL.

---

## 4. Thành viên 1 — Mai Khắc Hoàng Vũ (24120183)

**Nhiệm vụ:** Enhance Sysinfo system call · **Điểm:** 2 · **Nhánh:** `feature/sysinfo-memory`

**Yêu cầu chức năng**

- [Đề] `sysinfo()` trả thêm số byte bộ nhớ còn trống.
- [Quy ước nhóm] Giá trị = số trang trong `kmem.freelist` × `PGSIZE`, lấy tại thời điểm gọi, trả qua `si.freemem` (§2.3).
- Mọi chức năng Lab 1 giữ nguyên.

**Phạm vi công việc**

- `kernel/sysinfo.h`: thêm trường `freemem`.
- `kernel/kalloc.c`: hàm `kfreebytes()`. Có hai cách, chọn một và lập luận trong báo cáo:
  - (a) Duyệt `kmem.freelist` khi giữ `kmem.lock`: không đụng `kalloc`/`kfree`, chi phí O(số trang trống) mỗi lần gọi.
  - (b) Bộ đếm cập nhật trong `kalloc`/`kfree` dưới `kmem.lock`: O(1) khi đọc nhưng sửa đường nóng của allocator.
- `kernel/defs.h`, `kernel/sysproc.c` (thân `sys_sysinfo`), `user/freememtest.c`, `Makefile`, báo cáo.

**Kiến thức cần nghiên cứu**

- Free list nhúng trong chính trang trống (`struct run`), `kinit`/`freerange`, vùng `end`…`PHYSTOP`.
- Spinlock `kmem.lock`; chỉ cần mức "khi nào phải giữ lock" (sách mục 6.3, chỉ đọc phần liên quan).
- Những ai tiêu thụ trang: page-table pages, bộ nhớ user, trapframe, pipe buffer, buffer đối số trong `sys_exec`.
- Luồng `sbrk` → `growproc` → `uvmalloc`/`uvmdealloc` → `uvmunmap`; `fork` → `uvmcopy`.
- Thiết kế `sys_sysinfo` của Lab 1 (`kinfo` tĩnh, `kinfo_lock` là sleeplock).

**Chương 3:** 3.4, 3.5, 3.7 (bắt buộc); 3.6, 3.8 (nên đọc). Xem [docs/xv6_chapter3.md](docs/xv6_chapter3.md).

**File/hàm cần đọc:** `kernel/kalloc.c` (toàn bộ); `kernel/sysproc.c` (`sys_sysinfo`, `sys_sbrk`); `kernel/sysinfo.h`; `kernel/vm.c` (`walk`, `uvmalloc`, `uvmdealloc`, `uvmunmap`, `freewalk`); `kernel/proc.c` (`growproc`, `fork`, `freeproc`); `kernel/memlayout.h`; `user/sysinfotest.c` (`test_pointers`); `tests/sysinfo_fault.c`; `tests/file_info_native.c` (`preserved()`).

**File dự kiến sửa:** `kernel/sysinfo.h`, `kernel/kalloc.c`, `kernel/defs.h`, `kernel/sysproc.c`, `user/freememtest.c` (mới), `Makefile`.

**File dùng chung cần chú ý:** `kernel/sysproc.c` (Thi và Sang cũng thêm hàm; chỉ sửa trong thân `sys_sysinfo`); `kernel/defs.h`; `Makefile`; `kernel/sysinfo.h` (ABI với `sysinfotest`, `filetest`, `tests/*`).

**Trường hợp biên quan trọng**

1. CPU khác gọi `kalloc`/`kfree` trong lúc đếm → phải đếm (hoặc đọc bộ đếm) khi giữ `kmem.lock`.
2. Không giữ `kmem.lock` khi gọi `copyout` hoặc khi lấy lock khác. Lấy spinlock khi đang giữ sleeplock `kinfo_lock` là hợp lệ.
3. Giá trị chỉ là snapshot, có thể thay đổi ngay sau khi trả về.
4. Collector lỗi → `-1`, không `copyout`, không ghi gì vào buffer user.
5. Con trỏ user sai (NULL, vượt `MAXVA`, trang text chỉ đọc, ghi tràn 1 byte) vẫn trả `-1`. Struct to thêm 8 byte, các test biên của Lab 1 dùng `sizeof` nên vẫn đúng.
6. `0 ≤ freemem < PHYSTOP - KERNBASE` (128 MB) và chia hết cho `PGSIZE`; khi hết bộ nhớ thì `freemem = 0` vẫn hợp lệ.
7. `sbrk(-n)` chỉ giải phóng trang lá; page-table pages trung gian chỉ được trả khi tiến trình thoát hoặc `exec`. Vì vậy sau khi co, `freemem` có thể thấp hơn giá trị trước khi tăng vài trang.

**Yêu cầu kiểm thử (`freememtest`, chạy `CPUS=1` và `CPUS=3`)**

| ID | Kiểm tra | Kỳ vọng |
|---|---|---|
| F1 | `sysinfo` cơ bản | trả 0; `freemem > 0`; `freemem % 4096 == 0`; `freemem < 128 MB` |
| F2 | Hai lần gọi liên tiếp, không cấp phát | bằng nhau (khi hệ thống rảnh) |
| F3 | `sbrk(+N trang)` với `N ≤ 512` | giảm ≥ `N*4096` và ≤ `(N+3)*4096` (tính cả page-table pages) |
| F4 | `sbrk(-N trang)` | tăng đúng `N*4096` |
| F5 | `fork` khi con còn sống / sau `wait` | giảm khi con còn sống; trở lại đúng giá trị trước `fork` sau `wait` |
| F6 | Trường Lab 1 | `process_count ≥ 2`, tìm thấy PID của chính mình |
| F7 | Con trỏ không hợp lệ | `-1` (NULL, địa chỉ text, ≥ `MAXVA`) |
| F8 | (tùy chọn) cấp phát trong tiến trình con đến khi `sbrk` lỗi | `freemem` rất nhỏ; sau khi con thoát thì hồi phục |
| R | Hồi quy | `sysinfotest`, `filetest`, `forktest`, `usertests -q`; khuyến khích `tests/run_lab1_extra.py` |

**Deliverables:** code trên nhánh; `user/freememtest.c`; `docs/reports/24120183_sysinfo.md`; log/ảnh test với `CPUS=1` và `CPUS=3`; PR vào `integration/lab2`.

**Acceptance criteria**

- `freemem` nằm cuối struct; offset các trường Lab 1 không đổi.
- `freemem` khớp số trang trống × `PGSIZE`; biến thiên đúng như F3–F5.
- Không race: đọc dưới `kmem.lock`.
- Đường lỗi của Lab 1 giữ nguyên (`sysinfo_fault`).
- Test Lab 1 không bị sửa và vẫn PASS.
- Tuân thủ AGENTS.md §7.1: không có comment mới trong `kernel/` và `Makefile`; comment trong `user/freememtest.c` bằng tiếng Việt có dấu.

**Definition of Done:** AGENTS.md §11 + acceptance criteria ở trên + báo cáo giải thích vì sao chọn cách (a) hoặc (b).

---

## 5. Thành viên 2 — Hồ Sỹ Thi (24120452)

**Nhiệm vụ:** Print a page table · **Điểm:** 4 · **Nhánh:** `feature/vmprint`

**Yêu cầu chức năng**

- [Đề] `vmprint(pagetable_t)` in đúng định dạng §2.4:
  - dòng đầu là đối số;
  - một dòng cho mỗi PTE hợp lệ (kể cả PTE trỏ tới page-table page cấp dưới), thụt ` ..` theo độ sâu, gồm chỉ số, giá trị PTE và PA;
  - không in PTE không hợp lệ.
- [Đề] In bảng trang của tiến trình khi được thực thi, chỉ khi người dùng muốn; `exec` không đổi. Cơ chế: §2.4.
- [Đề] Báo cáo giải thích **từng trang lá**: nội dung logic và các bit quyền.

**Phạm vi công việc:** `vmprint` trong `vm.c`; prototype trong `defs.h`; lời gọi có điều kiện trong `exec.c`; trường cờ trong `proc.h` và đặt lại trong `freeproc()`; `sys_setvmprint` trong `sysproc.c`; đăng ký syscall 23 (`syscall.h`, `syscall.c`, `usys.pl`, `user.h`); `user/pgtbl.c`; `user/vmprinttest.c` (tùy chọn); `Makefile`; báo cáo.

**Kiến thức cần nghiên cứu**

- Cây bảng trang Sv39 ba cấp, 512 PTE/trang; `PX`, `PTE2PA`, `PTE_FLAGS`.
- Phân biệt PTE lá và PTE trỏ cấp dưới: hợp lệ nhưng không có R/W/X → trỏ xuống, giống quy tắc trong `freewalk()`.
- Bố cục không gian địa chỉ tiến trình (Hình 3.4): text, data, guard, stack, trapframe, trampoline; cách `exec` dựng bảng trang mới và thời điểm commit.
- Luồng một system call từ user tới kernel (sách mục 4.3–4.4, chỉ phần đăng ký syscall và lấy đối số).

**Chương 3:** 3.1 (Hình 3.2), 3.3, 3.6 (Hình 3.4), 3.8 (bắt buộc); 3.2 (Hình 3.3), bài tập 3.10.2 (nên đọc).

**File/hàm cần đọc:** `kernel/vm.c` (`walk`, `freewalk`, `uvmclear`); `kernel/riscv.h` (cuối file); `kernel/memlayout.h`; `kernel/exec.c` (toàn bộ); `kernel/sysfile.c` (`sys_exec`); `kernel/proc.c` (`allocproc`, `freeproc`, `proc_pagetable`, `fork`); `kernel/proc.h`; `kernel/syscall.c`, `kernel/syscall.h`; `user/usys.pl`, `user/user.h`; `kernel/printf.c` (`printptr`: `0x` + 16 chữ số hex).

**File dự kiến sửa:** `kernel/vm.c`, `kernel/defs.h`, `kernel/exec.c`, `kernel/proc.h`, `kernel/proc.c` (chỉ `freeproc`), `kernel/sysproc.c`, `kernel/syscall.h`, `kernel/syscall.c`, `user/usys.pl`, `user/user.h`, `user/pgtbl.c` (mới), `user/vmprinttest.c` (mới, tùy chọn), `Makefile`.

**File dùng chung cần chú ý:** `kernel/sysproc.c` (vị trí chèn ở §3); `kernel/defs.h`; các file registry (conflict dễ giải quyết với Sang); `kernel/proc.c` (chứa code Lab 1, chỉ thêm đúng 1 dòng trong `freeproc`).

**Trường hợp biên quan trọng**

1. Chỉ in PTE có `PTE_V`; không in, không đệ quy vào PTE không hợp lệ.
2. PTE lá ở cấp 2 hoặc 1 (superpage): xv6 không tạo, nhưng `vmprint` không được coi PA của trang dữ liệu là page-table page. In như trang lá và không đệ quy.
3. Độ sâu tối đa 3; không đệ quy dưới cấp 0.
4. `vmprint` chỉ đọc: không cấp phát (không gọi `walk(..., 1)`), không sửa PTE, không lấy lock có thể ngủ.
5. In **sau** khi commit (`p->pagetable` đã là bảng mới); đường `bad:` không in.
6. Cờ one-shot: nếu chính tiến trình đó `exec` tiếp một chương trình khác mà không gọi lại `setvmprint(1)` thì không in.
7. `fork` không kế thừa cờ, nên các lệnh mà một `sh` (mở bằng `pgtbl sh`) chạy qua `fork` + `exec` đều không in. `/init` lúc boot không in.
8. `pgtbl` không đối số → in hướng dẫn; chương trình không tồn tại → báo lỗi, không in bảng; đối số được truyền nguyên vẹn (`pgtbl echo a b` in `a b`).
9. Nhiều CPU: mỗi dòng nên in bằng một lời gọi `printf` để hạn chế xen kẽ output.
10. Output thực tế **khác** ví dụ của đề:
    - không có chỉ số 509 (trang `USYSCALL` của bài "speed up system calls" không thuộc lab này);
    - số entry cấp 0 phụ thuộc kích thước chương trình;
    - giá trị PA thay đổi theo lần chạy.

**Yêu cầu kiểm thử (`CPUS=1` và `CPUS=3`)**

| ID | Lệnh / kịch bản | Kỳ vọng |
|---|---|---|
| V1 | `echo hi` | chỉ in `hi`, không có bảng trang (exec không đổi) |
| V2 | `pgtbl echo hi` | in bảng trang, sau đó `hi` |
| V3 | Kiểm tra định dạng V2 | dòng đầu `page table 0x…`; thụt ` ..` đúng độ sâu; `%p` đủ 16 chữ số; không có PTE không hợp lệ |
| V4 | Đối chiếu cấp cao nhất | gốc idx 255 → idx 511 → idx 510 (trapframe, R W, không U) và idx 511 (trampoline, R X, không U, PA trùng symbol `trampoline` trong `kernel/kernel.sym`) |
| V5 | Đối chiếu text/data/guard/stack với `riscv64-unknown-elf-readelf -l user/_echo` (đổi tiền tố theo toolchain) | quyền khớp segment ELF; guard page không có U |
| V6 | `pgtbl ls`, `pgtbl cat README` | số trang lá thay đổi theo kích thước chương trình |
| V7 | `pgtbl sh`, rồi trong shell mới chạy `echo hi` | chỉ `exec` của `sh` in bảng; `echo` chạy qua `fork` nên không in |
| V8 | `vmprinttest` (tùy chọn, kiểm tra one-shot): `setvmprint(1)` rồi `exec` chính nó với đối số `stage2`; ở `stage2` `exec` `echo` mà không gọi lại `setvmprint` | bảng chỉ in đúng một lần (khi vào `stage2`) |
| V8b | `vmprinttest` (tùy chọn, kiểm tra `fork`): `setvmprint(1)` rồi `fork`, con `exec` `echo` | con không in |
| V9 | `pgtbl`, `pgtbl khongtontai` | hướng dẫn / lỗi, `exit(1)`, không in bảng |
| R | Hồi quy | `usertests -q` (gồm các test exec), `sysinfotest`, `filetest`, `forktest` |

**Deliverables:** code trên nhánh; `user/pgtbl.c` (+ `user/vmprinttest.c` nếu làm); `docs/reports/24120452_vmprint.md` có bảng giải thích **mọi** trang lá trong một output thật (VA, PTE, PA, cờ V/R/W/X/U, nội dung logic); log/ảnh; PR.

**Acceptance criteria**

- Định dạng khớp đề từng ký tự (trừ giá trị địa chỉ).
- Không in khi không yêu cầu; `exec`/`sys_exec` không đổi chữ ký và hành vi.
- One-shot và không kế thừa qua `fork`.
- Mọi trang lá được giải thích đúng.
- Toàn bộ hồi quy PASS.
- Tuân thủ AGENTS.md §7.1: không có comment mới trong `kernel/`, `user/pgtbl.c`, `Makefile` và các file registry; comment trong `user/vmprinttest.c` bằng tiếng Việt có dấu.

**Definition of Done:** AGENTS.md §11 + acceptance criteria ở trên.

---

## 6. Thành viên 3 — Đặng Quang Sang (24120428)

**Nhiệm vụ:** Detect which pages have been accessed · **Điểm:** 4 · **Nhánh:** `feature/pgaccess` · **Thêm:** review, tích hợp, nộp bài

**Yêu cầu chức năng**

- [Đề] System call `pgaccess(base, len, mask)` báo các trang đã được truy cập (đọc hoặc ghi) bằng bitmask, trang đầu là bit thấp nhất.
- [Đề] Định nghĩa `PTE_A` theo manual và kiểm soát việc set/clear bit này.
- [Quy ước nhóm] Ngữ nghĩa chi tiết theo §2.5.

**Phạm vi công việc:** `PTE_A` trong `riscv.h`; `sys_pgaccess` cuối `sysproc.c` (dùng `walk(..., 0)`, `argaddr`, `argint`, buffer tạm trong kernel, `copyout`); đăng ký syscall 24; `user/pgtbltest.c` (tự viết, không chép bản MIT); `Makefile`; báo cáo.

**Kiến thức cần nghiên cứu**

- Bit A/D trong PTE Sv39 (Privileged Spec, mục Sv32/Sv39 page-table entry): phần cứng (hoặc QEMU) đặt A khi page walker phân giải một truy cập. Spec cho phép hai cách: phần cứng tự cập nhật, hoặc sinh page fault để phần mềm xử lý. QEMU của nhóm theo cách nào thì kiểm chứng theo quy trình ở mục "Quy trình kiểm chứng bit A trên QEMU" bên dưới.
- TLB và `sfence.vma`: `userret` trong `trampoline.S` (dòng 108–110) flush TLB mỗi lần trở về user, nên sau khi kernel xóa `PTE_A`, lần truy cập tiếp theo sẽ đi qua page walker và đặt lại A. Đây là suy luận từ code; cần kiểm chứng bằng test.
- Kernel truy cập bộ nhớ user qua địa chỉ vật lý direct-map (`copyin`/`copyout`/`memset` trong `uvmalloc`), không qua bảng trang user, nên **không** làm bật `PTE_A` của trang user.
- `walk()` gọi `panic` khi `va >= MAXVA`; cần kiểm tra trước.
- Lấy đối số syscall (sách mục 4.4).

**Chương 3:** 3.1 (Hình 3.2), 3.3 (`walk`, TLB), 3.6, 3.7 (bắt buộc); 3.8 (guard page, nên đọc).

**File/hàm cần đọc:** `kernel/riscv.h`; `kernel/vm.c` (`walk`, `walkaddr`, `copyout`, `uvmalloc`, `uvmcopy`); `kernel/sysproc.c`; `kernel/syscall.c` (`argint`, `argaddr`, bảng syscall); `kernel/trampoline.S` (`userret`); `kernel/trap.c` (`usertrap`: page fault → giết tiến trình); `kernel/proc.c` (`growproc`); `kernel/exec.c` (guard page qua `uvmclear`); `user/user.h`, `user/usys.pl`.

**File dự kiến sửa:** `kernel/riscv.h`, `kernel/sysproc.c`, `kernel/syscall.h`, `kernel/syscall.c`, `user/usys.pl`, `user/user.h`, `user/pgtbltest.c` (mới), `Makefile`.

**File dùng chung cần chú ý:** `kernel/sysproc.c` (chỉ thêm ở cuối file); các file registry (conflict với Thi); `kernel/riscv.h` (header dùng chung, chỉ thêm 1 dòng).

**Trường hợp biên quan trọng**

1. `base` không chia hết `PGSIZE` (làm tròn xuống, không lỗi; ví dụ buffer từ `malloc`); `len ≤ 0`; `len > 64`.
2. Vùng vượt `p->sz`; `base` rất lớn (≥ `MAXVA`) → `-1` và **không panic**; tràn số khi tính `base + len*PGSIZE`.
3. `mask` NULL / chưa map / nằm trên trang text chỉ đọc → `-1`. Theo §2.5, thông tin A không bị mất.
4. `mask` nằm trong chính vùng đang kiểm tra: `copyout` của kernel không bật A, kết quả không bị ảnh hưởng.
5. Trang chưa từng truy cập → 0. Đọc và ghi đều bật A.
6. Gọi lần hai ngay sau đó → 0 (đã xóa A).
7. Guard page (không có `PTE_U`) → 0.
8. `len` không chia hết 8 (ví dụ 9): kernel ghi đúng 2 byte, byte canary phía sau không đổi.
9. `fork`: `uvmcopy` sao chép cả cờ của PTE (gồm A) sang con. Kết quả của con và cha độc lập sau đó.
10. Nhiều CPU: tiến trình có thể chạy trên CPU khác giữa các lần gọi. Chạy test với `CPUS=3`.

**Yêu cầu kiểm thử (`pgtbltest`, chạy `CPUS=1` và `CPUS=3`)**

| ID | Kịch bản | Kỳ vọng |
|---|---|---|
| P1 | `sbrk` 32 trang, gọi `pgaccess` để xóa, chạm trang 1, 2, 30, gọi lại | mask = bit 1, 2, 30 |
| P2 | Gọi lại ngay | mask = 0 |
| P3 | Chỉ đọc một trang / chỉ ghi một trang | cả hai đều được báo |
| P4 | `len = 64` | thành công; `len = 65`, `0`, `-1` → `-1` |
| P5 | `base` lệch trang; buffer từ `malloc(32*PGSIZE)` dùng như `pgaccess_test()` | báo đúng các trang tính từ trang chứa `base`, không lỗi |
| P6 | Vùng vượt `sbrk(0)`; `base = MAXVA`; `base = 0xffffffffffff0000` | `-1`, không panic |
| P7 | `mask` = 0, = địa chỉ text, = trang chưa map | `-1` |
| P8 | `len = 9` với canary | ghi đúng 2 byte |
| P9 | Vùng chứa guard page dưới stack | bit của guard page = 0 |
| P10 | `fork`: con chạm trang khác cha | mỗi bên chỉ thấy truy cập của mình (sau khi đã xóa) |
| P11 | Sau P7 (mask lỗi) gọi lại với mask hợp lệ | các truy cập trước đó vẫn được báo |
| P12 | Chạm lại một trang sau khi bit của nó đã bị xóa, gọi lại | bit đó được báo lại (pha D bên dưới) |
| P13 | `mask` nằm trong chính vùng đang kiểm tra (trường hợp biên 4) | mask ghi đúng; lần gọi sau không báo trang chứa mask |
| R | Hồi quy | `sysinfotest`, `filetest`, `forktest`, `usertests -q` |

**Quy trình kiểm chứng bit A trên QEMU**

Mục tiêu: xác định QEMU của nhóm có tự bật `PTE_A` bằng phần cứng hay không, và việc xóa `PTE_A` có hoạt động đúng với TLB hay không. Kết quả ghi vào báo cáo của Sang.

1. **Ghi môi trường.** Chạy `qemu-system-riscv64 --version` trong WSL và ghi lại phiên bản.
2. **Lập luận từ baseline** [Suy luận]:
   - `mappages()` (`vm.c:165`) tạo mọi PTE, cả kernel lẫn user, **không** kèm A/D.
   - Nếu QEMU theo kiểu "sinh page fault khi A = 0", lần nạp lệnh đầu tiên sau `kvminithart()` hoặc của `initcode` đã lỗi, và xv6 không thể boot tới shell.
   - Baseline boot được và PASS `usertests`. Vậy QEMU của nhóm **tự cập nhật A/D bằng phần cứng**. Bước 3 xác nhận điều này bằng thực nghiệm.
3. **Thực nghiệm trong `pgtbltest`.** Dùng một vùng `sbrk` mới (ví dụ 32 trang) cho cả 4 pha. Biến `mask` đặt ngoài vùng này.

   | Pha | Thao tác | Kỳ vọng | Nếu sai thì nghĩa là |
   |---|---|---|---|
   | A | Không chạm vùng mới, gọi `pgaccess` | mask = 0 | Kernel ghi trang qua direct map đã làm bật A (sai giả định), hoặc đọc nhầm bit |
   | B | Đọc trang 1, ghi trang 2 và 30, gọi `pgaccess` | chỉ bit 1, 2, 30 bằng 1 | Phần cứng không bật A, `PTE_A` sai giá trị, hoặc sai thứ tự bit |
   | C | Gọi lại ngay, không chạm | mask = 0 | Chưa xóa `PTE_A` |
   | D | Chạm lại trang 2, gọi `pgaccess` | chỉ bit 2 | TLB còn entry cũ nên page walker không chạy lại; kiểm tra `sfence.vma` trong `userret` (`trampoline.S:108–110`) |

4. **Chạy với `CPUS=1` và `CPUS=3`.** Với 3 CPU, tiến trình có thể chuyển CPU giữa các pha. Pha D vẫn phải đúng vì mọi CPU đều flush TLB khi về user.
5. **(Tùy chọn, chỉ để debug, không commit.)** Sau khi đã merge `vmprint`, có thể tạm gọi `vmprint(p->pagetable)` trong `sys_pgaccess` trước bước xóa. Khi đó sẽ thấy cờ của trang đã đọc có `0x40` (A), trang đã ghi có thêm `0x80` (D). Xóa lời gọi này trước khi commit.
6. **Ghi vào báo cáo:**
   - phiên bản QEMU;
   - kết quả 4 pha ở cả hai cấu hình CPU;
   - kết luận: phần cứng tự cập nhật A, hay hiện tượng khác (nếu có).

**Deliverables:** code; `user/pgtbltest.c`; `docs/reports/24120428_pgaccess.md` (gồm giải thích bit A, chiến lược set/clear, lý giải TLB, kết quả kiểm chứng trên QEMU); log/ảnh; PR.

**Acceptance criteria**

- Ngữ nghĩa đúng §2.5.
- Không panic với mọi đầu vào từ user.
- `PTE_A = 1L << 6`, có trích dẫn spec.
- Kết quả P1–P12 PASS ở cả hai cấu hình CPU.
- Không sửa file ngoài phạm vi.
- Tuân thủ AGENTS.md §7.1: không có comment mới trong `kernel/`, `Makefile` và các file registry; comment trong `user/pgtbltest.c` bằng tiếng Việt có dấu.

**Definition of Done:** AGENTS.md §11 + acceptance criteria ở trên + hoàn thành vai trò tích hợp (§7).

**Trách nhiệm tích hợp (Sang)**

- Tạo `integration/lab2` từ `main` và push lên remote.
- Review từng PR:
  - phạm vi file (`git diff integration/lab2...feature/<x> --stat`);
  - quy định comment (AGENTS.md §7.1): chạy lệnh kiểm tra comment với `lab2-baseline origin/feature/<x>` thay cho `lab2-baseline`, kết quả phải rỗng;
  - không có CRLF hay format thừa;
  - có log test thật.
- Merge theo thứ tự Vũ → Thi → Sang. Sau mỗi lần merge: `make clean`, build, chạy bộ hồi quy với `CPUS=1` và `CPUS=3`.
- Giải quyết conflict registry theo §3. Nếu phải sửa code của người khác, ghi lại và báo người đó.

---

## 7. Kiểm thử tích hợp và đóng gói nộp bài (Sang)

1. Trên commit cuối của `integration/lab2`, `make clean` rồi chạy đủ: `sysinfotest`, `filetest`, `forktest`, `usertests -q`, `freememtest`, `pgtbl echo hi`, `echo hi`, `vmprinttest` (nếu có), `pgtbltest`, với `CPUS=1` và `CPUS=3`. Khuyến khích chạy thêm `tests/run_lab1_extra.py`.
2. Tổng hợp ba báo cáo thành `24120183_24120428_24120452_Report.pdf` (ngắn gọn, không dán source).
3. Tạo patch và source từ **cùng một commit**. Bản nền tạo patch là tag `lab2-baseline` (đã chốt, xem §8):

```bash
git switch integration/lab2          # đúng commit đã test
git status --short                   # phải sạch
make clean
# Patch: chỉ phần mã nguồn, so với lab2-baseline
git diff lab2-baseline HEAD -- kernel user mkfs Makefile > 24120183_24120428_24120452.patch
# Source sạch: chỉ file được Git theo dõi, không có file build và tài liệu quản lý
git archive --format=zip -o Source.zip HEAD \
  kernel user mkfs tests Makefile README LICENSE \
  .gitignore .editorconfig .dir-locals.el .gdbinit.tmpl-riscv
```

4. Kiểm tra lại:
   - lệnh kiểm tra comment (AGENTS.md §7.1) trên commit cuối không in ra dòng nào;
   - giải nén `Source.zip` vào thư mục trống, `make qemu` chạy được;
   - patch áp được lên bản nền sạch:

     ```bash
     rm -rf /tmp/xv6-base && mkdir -p /tmp/xv6-base
     git archive lab2-baseline | tar -x -C /tmp/xv6-base
     (cd /tmp/xv6-base && git apply --check "$OLDPWD/24120183_24120428_24120452.patch")
     ```

   - trong patch và zip không có `AGENTS.md`, `CLAUDE.md`, `TASK_ASSIGNMENT.md`, `REPORT_TEMPLATE.md`, `docs/`, file build.
5. Đóng gói `24120183_24120428_24120452.zip` gồm Report.pdf, file `.patch`, `Source.zip`; nộp trên Moodle.

---

## 8. Các quyết định đã chốt (26/11/2026)

| # | Vấn đề | Quyết định |
|---|---|---|
| 1 | Bản nền để tạo patch | So với tag `lab2-baseline` (Lab 1 hoàn chỉnh). Lệnh ở §7 |
| 2 | Có đưa thư mục `tests/` (test Lab 1) vào Source.zip? | Có, vì đây là mã nguồn. Đã nằm trong lệnh `git archive` ở §7 |
| 3 | QEMU tự bật bit A bằng phần cứng hay sinh page fault? | Kiểm chứng theo "Quy trình kiểm chứng bit A trên QEMU" ở §6. Sang ghi kết quả vào báo cáo |
| 4 | Ví dụ vmprint trong đề có index 509 (trang `USYSCALL`) và lời văn ghi "0, 1, 2" trong khi hình in 0–3 | Báo cáo của Thi ghi chú: output của nhóm phản ánh bố cục thật của repo, không có trang `USYSCALL` vì phần "speed up system calls" không thuộc yêu cầu chấm |
| 5 | Có in bảng trang của `init` lúc boot như bản MIT? | Không. Đề yêu cầu chỉ in khi người dùng muốn; báo cáo của Thi nêu giả định này |
| 6 | Giới hạn 64 trang và cách xử lý `base` lệch trang của `pgaccess` | Là quy ước nhóm (§2.5): tối đa 64 trang; `base` lệch trang được làm tròn xuống. Ban đầu nhóm bắt `base` thẳng trang; sau khi đối chiếu `Lab2_Memory.pdf` thấy `pgaccess_test()` mà đề gợi ý dùng buffer từ `malloc` nên đổi (26/11/2026). Báo cáo của Sang nêu rõ |
