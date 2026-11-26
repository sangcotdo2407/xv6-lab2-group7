# AGENTS.md — Hướng dẫn chung cho coding agents (xv6 Lab 2 – Memory)

Tài liệu này áp dụng cho **mọi** coding agent (Claude Code, Codex, Copilot, …) và mọi thành viên khi làm việc trong repository `xv6-lab2-group7`. Đọc hết mục 0 trước khi sửa bất kỳ file nào.

Tài liệu liên quan:

- [TASK_ASSIGNMENT.md](TASK_ASSIGNMENT.md): phân công chi tiết và **quy ước dùng chung** (số hiệu syscall, tên hàm, file được sửa). Khi hai tài liệu mâu thuẫn về quy ước kỹ thuật, theo TASK_ASSIGNMENT.md §2.
- [docs/xv6_chapter3.md](docs/xv6_chapter3.md): tóm lược Chương 3 sách xv6 kèm đối chiếu mã nguồn.
- [REPORT_TEMPLATE.md](REPORT_TEMPLATE.md): mẫu báo cáo.

---

## 0. Quy tắc bắt buộc (đọc trước tiên)

1. Tuân thủ quy định comment ở §7.1: không thêm comment mới vào file mã nguồn chức năng; giữ nguyên comment mặc định của xv6 và comment có sẵn từ Lab 1; comment mới trong file kiểm thử viết bằng tiếng Việt có dấu.
2. Không sửa file hoặc hàm nằm ngoài phạm vi nhiệm vụ được giao (xem ma trận file ở TASK_ASSIGNMENT.md §3) nếu không có lý do kỹ thuật rõ ràng. Khi bắt buộc phải sửa ngoài phạm vi, dừng lại và hỏi người dùng trước.
3. Không refactor, không đổi tên, không format hàng loạt, không sắp xếp lại include, không đổi khoảng trắng của dòng không liên quan.
4. Không làm mất chức năng Lab 1: `sysinfo()` (thống kê file/process), `scheduling_count`, `collect_file_info()`, `collect_process_info()`, `file_index_of()`, `filesetinfo()`, các chương trình `sysinfotest`, `filetest` và bộ test trong `tests/`.
5. Không thay đổi hành vi mặc định của `exec()`: chữ ký `int exec(char*, char**)`, `sys_exec()`, giá trị trả về, bố cục bộ nhớ và output khi người dùng không yêu cầu in bảng trang.
6. Tuân thủ phong cách xv6: thụt lề 2 dấu cách cho `.c/.h` (xem `.editorconfig`), kiểu trả về nằm trên dòng riêng khi định nghĩa hàm, `if(` không có dấu cách, dùng `uint64`, `pte_t`, `pagetable_t`, `PGSIZE`, `PGROUNDDOWN`… sẵn có.
7. Kiểm tra tính đúng đắn, đồng bộ (lock), an toàn bộ nhớ (con trỏ từ user, tràn số, vượt `MAXVA`, vượt `p->sz`) và các trường hợp biên trước khi coi là xong.
8. Không cài đặt lại thứ đã có (`walk()`, `copyout()`, `argaddr()`, `argint()`, `PGROUNDDOWN`, `PTE2PA`, `PX`…). Dùng lại hàm hiện có.
9. Không sao chép nguyên văn lời giải có sẵn trên mạng (kể cả nhánh `pgtbl` của MIT 6.1810). Đề cảnh báo: bài giống nhau bị 0 điểm toàn bộ phần thực hành.
10. Không tự ý `git commit`, `git push`, `git merge`, `git rebase`, `git push --force`, tạo/xóa branch hoặc tag. Chỉ thực hiện khi người dùng yêu cầu rõ ràng trong phiên hiện tại.
11. Không báo cáo test PASS nếu chưa thực sự chạy và nhìn thấy kết quả. Ghi rõ test nào chưa chạy được và lý do.
12. Không đưa file build (`*.o`, `*.d`, `*.asm`, `*.sym`, `kernel/kernel`, `fs.img`, `user/_*`, `user/usys.S`), cache, log, `test-results/` hoặc tài liệu quản lý (`AGENTS.md`, `CLAUDE.md`, `TASK_ASSIGNMENT.md`, `REPORT_TEMPLATE.md`, `docs/`) vào gói source nộp bài.
13. Mọi thay đổi phải giải thích được khi vấn đáp: mỗi dòng code mới phải trả lời được "dòng này làm gì, tại sao cần, bỏ đi thì sao".
14. Không xóa, di chuyển hoặc tạo lại tag `lab2-baseline`.
15. Không thay đổi ABI hoặc quy ước dữ liệu dùng chung (số hiệu syscall, layout `struct sysinfo`, prototype trong `defs.h`/`user.h`, tên chương trình test) ngoài những gì đã chốt ở TASK_ASSIGNMENT.md §2.
16. Khi yêu cầu mơ hồ, nêu rõ giả định và lý do chọn giải pháp (trong câu trả lời và trong báo cáo). Không tự tạo thêm yêu cầu ngoài đề.

---

## 1. Tổng quan dự án

| Mục | Giá trị |
|---|---|
| Repository | `xv6-lab2-group7` (remote `origin`: GitHub) |
| Mã nguồn nền | xv6-labs-2023 (xv6-riscv), kế thừa **toàn bộ** source hoàn chỉnh của Lab 1 |
| Nhánh mặc định | `main` |
| Baseline | commit `cd1e597`, tag `lab2-baseline` |
| Môi trường | VS Code + Ubuntu WSL + QEMU (`qemu-system-riscv64`), toolchain `riscv64-*-gcc` |
| Trạng thái baseline | Build thành công; PASS `sysinfotest`, `filetest`, `forktest`, `usertests -q` (theo xác nhận của nhóm) |

Thành viên và nhiệm vụ:

| Thành viên | MSSV | Nhiệm vụ | Điểm | Nhánh |
|---|---|---|---|---|
| Mai Khắc Hoàng Vũ | 24120183 | Enhance Sysinfo system call | 2 | `feature/sysinfo-memory` |
| Hồ Sỹ Thi | 24120452 | Print a page table | 4 | `feature/vmprint` |
| Đặng Quang Sang | 24120428 | Detect which pages have been accessed + tích hợp, nộp bài | 4 | `feature/pgaccess` |

---

## 2. Yêu cầu chính thức của Lab 2 (tóm tắt từ `Lab2_Memory.pdf`)

Trước khi code: đọc Chương 3 sách xv6, `kernel/memlayout.h`, `kernel/vm.c`, `kernel/kalloc.c`; có thể tham khảo RISC-V Privileged Architecture Manual.

1. **Enhance Sysinfo (2 điểm)**: mở rộng system call `sysinfo` của Lab 1 để trả về thêm **số byte bộ nhớ còn trống**.
2. **Print a page table (4 điểm)**:
   - Định nghĩa `vmprint()` trong kernel, nhận `pagetable_t`, in theo đúng định dạng đề:
     - Dòng đầu: `page table <đối số dạng %p>`.
     - Mỗi PTE hợp lệ (kể cả PTE trỏ tới page-table page cấp dưới) một dòng; thụt bằng các cụm ` ..` theo độ sâu; in chỉ số PTE trong trang, giá trị PTE (`%p`) và địa chỉ vật lý trích từ PTE (`%p`).
     - Không in PTE không hợp lệ.
   - Cung cấp cách in bảng trang của tiến trình **khi nó được thực thi** mà **không ảnh hưởng** `exec` hiện có: `exec` vẫn chạy chương trình bình thường, bảng trang chỉ được in **khi người dùng muốn**.
   - Gợi ý của đề: xem `sys_exec` (`kernel/sysfile.c`) và `exec()` (`kernel/exec.c`); tham khảo `walk()`, `freewalk()`; đặt `vmprint()` trong `kernel/vm.c`; gọi `vmprint(p->pagetable)` trong `exec.c` ngay trước `return argc`; dùng macro cuối `kernel/riscv.h`; khai báo prototype trong `kernel/defs.h`; dùng `%p`.
   - Báo cáo: với **mỗi trang lá** trong output, giải thích trang đó chứa gì về mặt logic và các bit quyền của nó.
3. **Detect which pages have been accessed (4 điểm)**:
   - Cài đặt system call `pgaccess()` gồm 3 đối số: địa chỉ ảo bắt đầu của trang user đầu tiên; số trang cần kiểm tra; địa chỉ user của buffer nhận **bitmask** (1 bit/trang, trang đầu ứng với bit thấp nhất).
   - Gợi ý: bắt đầu từ `sys_pgaccess()` trong `kernel/sysproc.c`; dùng `argaddr()`, `argint()`; dùng buffer tạm trong kernel rồi `copyout()`; được phép đặt giới hạn số trang; dùng `walk()`; tự định nghĩa `PTE_A` trong `kernel/riscv.h` theo manual; phải kiểm soát việc set/clear `PTE_A`; có thể dùng `vmprint()` để debug.
   - Kiểm thử bằng `pgtbltest` có sẵn hoặc chương trình tự viết. **Lưu ý: repo này không có `user/pgtbltest.c`** (xem §4).

Nộp bài (Moodle, không nhận qua email): file `24120183_24120428_24120452.zip` (MSSV tăng dần) gồm `24120183_24120428_24120452_Report.pdf` (ngắn gọn, không dán source code), file patch (`git diff > 24120183_24120428_24120452.patch`) và file zip source xv6 đã `make clean`.

Vấn đáp: chấm theo mức độ hoàn thiện, tính đúng, phong cách lập trình, độ kỹ của kiểm thử, lời giải và mức hiểu code; đánh giá cá nhân theo đóng góp.

---

## 3. Kiến trúc và thành phần mã nguồn liên quan

Số dòng lấy tại baseline `cd1e597`; có thể lệch sau khi sửa.

| File | Thành phần | Liên quan |
|---|---|---|
| `kernel/riscv.h` | `PTE_V/R/W/X/U` (341–345), `PA2PTE`, `PTE2PA`, `PTE_FLAGS` (348–352), `PX` (357), `MAXVA` (363), `sfence_vma()` | Cả 3 task. **Chưa có `PTE_A`**. |
| `kernel/memlayout.h` | `KERNBASE`, `PHYSTOP` (128 MB), `TRAMPOLINE`, `TRAPFRAME`, `KSTACK` | vmprint, sysinfo |
| `kernel/vm.c` | `kvmmake` (20), `walk` (86), `walkaddr` (109), `mappages` (144), `uvmunmap` (178), `uvmalloc` (233), `uvmdealloc` (263), `freewalk` (279), `uvmfree` (299), `uvmcopy` (313), `uvmclear` (345), `copyout` (359), `copyin` (389), `copyinstr` (415) | vmprint, pgaccess |
| `kernel/kalloc.c` | `struct run` (17), `kmem {lock, freelist}` (21–24), `kinit` (27), `freerange` (34), `kfree` (47), `kalloc` (69) | sysinfo |
| `kernel/exec.c` | `flags2perm` (12), `exec` (23), commit page table (124–129), `return argc` (131), nhãn `bad` (133) | vmprint |
| `kernel/sysfile.c` | `sys_exec` (440) | vmprint (chỉ đọc) |
| `kernel/sysproc.c` | `sys_sbrk` (41), `kinfo`/`kinfo_lock` (99–100), `sys_sysinfo` (103–125) | sysinfo, pgaccess, vmprint |
| `kernel/syscall.h` / `syscall.c` | `SYS_fork`=1 … `SYS_sysinfo`=22; bảng `syscalls[]` (108–131); `argint`, `argaddr` | vmprint, pgaccess |
| `kernel/sysinfo.h` | `struct sysinfo_file`, `sysinfo_fd`, `sysinfo_process`, `sysinfo` | sysinfo (ABI dùng chung kernel/user) |
| `kernel/proc.c` / `proc.h` | `allocproc` (111), `freeproc` (158), `proc_pagetable` (180), `proc_freepagetable` (214), `growproc` (263), `fork` (283), `collect_process_info` (670); `struct proc` | vmprint (cờ in), tham khảo |
| `kernel/file.c` | `collect_file_info`, `file_index_of` (Lab 1) | Không được sửa |
| `kernel/trampoline.S` | `userret`: `sfence.vma` + `csrw satp` (108–110) | pgaccess (lý giải TLB) |
| `kernel/defs.h` | Prototype các module (mục `// kalloc.c` ~67, `// vm.c` ~163) | cả 3 task |
| `user/user.h`, `user/usys.pl` | Prototype và stub syscall phía user | vmprint, pgaccess |
| `Makefile` | `UPROGS` (118–136), `CPUS := 3` mặc định | cả 3 task (thêm chương trình test) |
| `user/sysinfotest.c`, `user/filetest.c`, `tests/*` | Kiểm thử Lab 1 | Chạy hồi quy, **không sửa** |

---

## 4. Hiện trạng source nền (đã khảo sát)

| Thành phần | Trạng thái |
|---|---|
| `walk()`, `freewalk()`, `copyout()`, `argint()`, `argaddr()` | Có sẵn, dùng lại |
| `PTE_A` | **Chưa có** (Sang định nghĩa) |
| `vmprint()` | **Chưa có** (Thi cài đặt) |
| `pgaccess()` / `sys_pgaccess` | **Chưa có** (Sang cài đặt) |
| `user/pgtbltest.c` | **Không tồn tại** (repo không xuất phát từ nhánh `pgtbl` của MIT). Sang tự viết. |
| Trường bộ nhớ trống trong `struct sysinfo` | **Chưa có** (Vũ bổ sung) |
| Trang `USYSCALL`/`ugetpid` (phần "speed up system calls" của MIT) | Không có, **không thuộc yêu cầu chấm**, không cài đặt |
| Target `make grade` | Không có; kiểm thử thủ công trong QEMU |
| `tests/run_lab1_extra.py`, `run_file_info.py`, `run_process_info.py` | Kiểm thử Lab 1 tự động (Python + toolchain trong WSL), dùng cho hồi quy |

---

## 5. Thứ tự ưu tiên tài liệu

1. Đề bài `Lab2_Memory.pdf` của giảng viên.
2. Mã nguồn thực tế trong repository (luôn đúng hơn sách khi mô tả code hiện tại).
3. [TASK_ASSIGNMENT.md](TASK_ASSIGNMENT.md) (quy ước nhóm) và tài liệu này.
4. Sách xv6 RISC-V (bản 31/08/2024), tóm lược trong [docs/xv6_chapter3.md](docs/xv6_chapter3.md).
5. RISC-V Privileged Architecture Manual (định nghĩa bit PTE, đặc biệt A/D).
6. Nguồn khác (chỉ để hiểu khái niệm, không chép code).

Lưu ý: sách bản 2024 có một số số dòng và chi tiết khác repo (xem docs/xv6_chapter3.md, mục "Khác biệt giữa sách (2024) và repo").

---

## 6. Đọc chọn lọc Chương 3 theo nhiệm vụ

| Nhiệm vụ | Bắt buộc | Nên đọc thêm |
|---|---|---|
| Sysinfo (Vũ) | 3.4, 3.5, 3.7 | 3.6 (ai tiêu thụ trang vật lý), 3.8 (exec cấp phát gì) |
| Vmprint (Thi) | 3.1 (Hình 3.2), 3.3, 3.6 (Hình 3.4), 3.8 | 3.2 (Hình 3.3), bài tập 3.10.2 |
| Pgaccess (Sang) | 3.1 (Hình 3.2, bit A), 3.3 (`walk`, TLB), 3.6, 3.7 | 3.8 (trang guard), privileged spec mục Sv39 |

Chi tiết từng mục và đối chiếu code: [docs/xv6_chapter3.md](docs/xv6_chapter3.md).

---

## 7. Quy tắc lập trình

- Thay đổi tối thiểu, đúng chỗ; mỗi thay đổi phục vụ trực tiếp yêu cầu của đề.
- Code mới đặt tại vị trí đã chốt ở TASK_ASSIGNMENT.md §3 để giảm conflict.
- Không `#include` thừa, không thêm macro/hàm tiện ích không dùng.
- Mọi con trỏ/địa chỉ từ user là **không tin cậy**: chỉ truy cập qua `copyin`/`copyout`; kiểm tra `va < MAXVA` **trước** khi gọi `walk()` (vì `walk()` gọi `panic` khi `va >= MAXVA`); gọi `walk(..., 0)` (không cấp phát) khi chỉ tra cứu.
- Kiểm tra tràn số khi tính `base + n*PGSIZE`.
- Không giữ spinlock khi gọi hàm có thể ngủ hoặc `copyout`; tuân theo thứ tự lock hiện có (`wait_lock` trước `p->lock`; không giữ hai `p->lock`).
- Không cấp phát bộ nhớ lớn trên kernel stack (chỉ 1 trang); dùng biến cục bộ nhỏ hoặc `static` có bảo vệ như Lab 1 đã làm với `kinfo`.
- Giữ line ending **LF** (repo dùng LF; Windows có thể tự chuyển CRLF — kiểm tra `git diff` không có thay đổi toàn file).
- Build phải sạch với `-Wall -Werror` của Makefile.

### 7.1. Quy định comment trong mã nguồn (Source Code Comment Policy) — bắt buộc

Phân loại file:

| Loại | File | Comment mới |
|---|---|---|
| Mã nguồn chức năng | Mọi file `.c`, `.h`, `.S`, `.ld`, `.pl` trong `kernel/`, `user/`, `mkfs/` và `Makefile` ở thư mục gốc, không thuộc hai nhóm dưới, **kể cả** chương trình user phục vụ tính năng như `user/pgtbl.c` | Không được thêm |
| File kiểm thử Lab 2 | Chương trình test mới ở TASK_ASSIGNMENT.md §2.6: `user/freememtest.c`, `user/vmprinttest.c`, `user/pgtbltest.c` | Được thêm, bằng tiếng Việt có dấu |
| Test có sẵn | `user/sysinfotest.c`, `user/filetest.c`, `user/usertests.c`, `user/forktest.c`, `tests/` | Không sửa file (TASK_ASSIGNMENT.md §3) |

1. **Mã nguồn chức năng:**
   - Không thêm comment mới dưới bất kỳ dạng nào (`//`, `/* */`, comment cuối dòng, `#` trong `Makefile`/`.pl`).
   - Giữ nguyên toàn bộ comment mặc định của xv6 và comment có sẵn từ Lab 1: không xóa, dịch, viết lại, di chuyển hay đổi định dạng.
   - Nếu comment cũ trở nên sai do thay đổi logic bắt buộc: không tự sửa; báo cho thành viên phụ trách quyết định và ghi lại trong mô tả PR.
   - Code mới phải tự giải thích qua tên biến, tên hàm, tên macro và cấu trúc rõ ràng. Lý do thiết kế ghi trong báo cáo cá nhân (`docs/reports/`), không ghi trong code.
2. **File kiểm thử:**
   - Được thêm comment giải thích mục đích, dữ liệu đầu vào, kết quả mong đợi và trường hợp kiểm thử.
   - Comment mới viết bằng tiếng Việt có dấu, mã hóa UTF-8 (không BOM); giữ nguyên thuật ngữ kỹ thuật tiếng Anh (PTE, page table, bitmask, `sbrk`…).
   - Không dịch hoặc xóa comment đã có trong file kiểm thử.
   - Không thêm comment dư thừa hoặc giải thích câu lệnh hiển nhiên.
3. **Kiểm tra trước khi hoàn thành bất kỳ nhiệm vụ nào** (agent và thành viên):
   - Phát hiện comment bị thêm, xóa hoặc sửa trong file chức năng. Kết quả đúng là **không in ra dòng nào**:

     ```bash
     git diff -U0 lab2-baseline -- kernel user mkfs Makefile \
         ':!user/freememtest.c' ':!user/vmprinttest.c' ':!user/pgtbltest.c' \
       | grep -vE '^(\+\+\+|---) ' \
       | grep -E '^[+-].*(//|/\*|\*/)|^[+-][[:space:]]*# '
     ```

   - Với file kiểm thử: đọc lại mọi comment mới (tiếng Việt có dấu, không dư thừa); `file <tên file>` phải báo UTF-8 và không có CRLF.
   - Loại bỏ mọi comment mới không được phép trước khi kết thúc.
   - Agent phải nêu trong phần tổng kết/review cuối cùng: đã chạy kiểm tra comment, kết quả, và comment nào đã bị loại bỏ (nếu có).

---

## 8. Quy tắc Git và phạm vi chỉnh sửa

- `main`: source nền + tài liệu chung. Không code tính năng trực tiếp trên `main`.
- Nhánh tính năng tạo từ `main` mới nhất: `feature/sysinfo-memory`, `feature/vmprint`, `feature/pgaccess`.
- Nhánh tích hợp: `integration/lab2` (Sang tạo từ `main`). PR của từng người hướng vào `integration/lab2`.
- Commit message theo kiểu hiện có (Conventional Commits): `feat(sysinfo): ...`, `feat(vmprint): ...`, `feat(pgaccess): ...`, `test(...): ...`, `docs(report): ...`, `fix(...): ...`.
- Mỗi commit nhỏ, build được. Không commit file build (đã có trong `.gitignore`), không commit `fs.img`.
- Agent **chỉ được** sửa file trong phạm vi của thành viên đang làm việc (TASK_ASSIGNMENT.md §3). Agent không tự commit/push/merge (quy tắc 10).
- Trước khi kết thúc phiên: chạy `git status --short` và `git diff --stat`, rà từng hunk, đảm bảo không có thay đổi ngoài phạm vi, không có thay đổi khoảng trắng/CRLF; chạy kiểm tra comment ở §7.1.
- Để xem riêng thay đổi Lab 2: `git diff lab2-baseline -- kernel user Makefile`.

---

## 9. Quy tắc kiểm thử

Build và chạy (trong WSL):

```bash
make clean            # bắt buộc khi đổi header dùng chung (sysinfo.h, riscv.h, syscall.h)
make qemu CPUS=1      # cấu hình 1 CPU
make qemu             # cấu hình mặc định CPUS=3
# thoát QEMU: Ctrl-a rồi x
```

Bộ kiểm thử tối thiểu cho **mỗi** nhánh trước khi tạo PR (chạy với cả `CPUS=1` và `CPUS=3`):

| Nhóm | Lệnh trong shell xv6 | Kỳ vọng |
|---|---|---|
| Hồi quy Lab 1 | `sysinfotest` | Dòng cuối `===== N/N PASS =====` |
| Hồi quy Lab 1 | `filetest` | `filetest: ALL PASSED` |
| Hồi quy xv6 | `forktest` | `fork test OK` |
| Hồi quy xv6 | `usertests -q` | `ALL TESTS PASSED` |
| Tính năng | chương trình test của nhiệm vụ (TASK_ASSIGNMENT.md §2.6) | Tất cả kiểm tra PASS |

Tùy chọn (khuyến khích trước khi nộp): `python3 tests/run_lab1_extra.py`, `tests/run_file_info.py`, `tests/run_process_info.py` (cần toolchain trong WSL; kết quả ghi vào `test-results/`, không commit).

Quy định:

- Test phải bao gồm trường hợp bình thường, trường hợp biên và đầu vào không hợp lệ.
- Chương trình test tự in `PASS`/`FAIL` cho từng kiểm tra và tổng kết cuối; trả `exit(1)` nếu có lỗi.
- Ghi lại output thật (copy log hoặc ảnh chụp). Không chỉnh sửa, không "làm đẹp" kết quả.
- Không sửa test Lab 1 để test mới qua được.

---

## 10. Quy tắc viết báo cáo

- Mỗi thành viên sao chép [REPORT_TEMPLATE.md](REPORT_TEMPLATE.md) thành file riêng trong `docs/reports/` (tên file ở TASK_ASSIGNMENT.md §2.7). **Không sửa trực tiếp** REPORT_TEMPLATE.md.
- Báo cáo ngắn gọn ("short and sweet"), không dán toàn bộ source code; chỉ trích đoạn ngắn khi cần giải thích.
- Ghi rõ giả định, quyết định thiết kế, cái chưa làm được và hướng đã thử nhưng thất bại (giúp được điểm từng phần).
- Kết quả kiểm thử phải là kết quả thật kèm minh chứng.
- Sang tổng hợp ba báo cáo thành `24120183_24120428_24120452_Report.pdf`.

---

## 11. Tiêu chí hoàn thành (Definition of Done chung)

Một nhiệm vụ chỉ được coi là xong khi:

- [ ] Đáp ứng đủ yêu cầu đề bài của nhiệm vụ và quy ước ở TASK_ASSIGNMENT.md §2.
- [ ] `make clean && make qemu` build không warning/error.
- [ ] Toàn bộ bộ kiểm thử ở §9 PASS với `CPUS=1` và `CPUS=3`, có log/ảnh minh chứng.
- [ ] Chỉ sửa các file trong phạm vi; diff sạch (không format, không CRLF).
- [ ] Tuân thủ quy định comment §7.1: kiểm tra comment không in ra dòng nào; comment trong file kiểm thử bằng tiếng Việt có dấu.
- [ ] Báo cáo cá nhân trong `docs/reports/` hoàn chỉnh theo template.
- [ ] Thành viên tự giải thích được mọi dòng code đã thay đổi.
- [ ] Đã push nhánh tính năng và tạo PR vào `integration/lab2` (do thành viên thực hiện, không phải agent).

---

## 12. Quy định sử dụng AI

- AI được dùng để giải thích code, gợi ý thiết kế, viết nháp code/test, rà lỗi. **Thành viên chịu trách nhiệm cuối cùng** về mọi dòng code và phải hiểu để vấn đáp.
- Trước khi giao việc cho AI: cung cấp phạm vi file được sửa và quy ước ở TASK_ASSIGNMENT.md §2.
- Agent phải đọc code thật trong repo trước khi sửa; không giả định theo bản xv6 khác hoặc theo lời giải trên mạng.
- Agent không được: chạy lệnh Git làm thay đổi lịch sử/remote, xóa file, sửa test Lab 1, tạo kết quả test giả, tự mở rộng yêu cầu.
- Mọi output của AI phải được review: đọc diff, build, chạy test.
- Nếu môn học yêu cầu khai báo việc dùng AI, ghi trong mục "Khó khăn và cách giải quyết" của báo cáo.

---

## 13. Đóng gói nộp bài (chỉ Sang thực hiện)

Xem TASK_ASSIGNMENT.md §7. Nguyên tắc:

- Patch và Source.zip phải sinh từ **cùng một commit cuối** trên `integration/lab2`.
- Loại bỏ file build và tài liệu quản lý.
- Patch tạo bằng cách so với tag `lab2-baseline` (đã chốt).
- Source.zip gồm cả thư mục `tests/` của Lab 1.
