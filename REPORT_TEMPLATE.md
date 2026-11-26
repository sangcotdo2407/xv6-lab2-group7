# Mẫu báo cáo cá nhân — Lab 2 (Memory)

> **Cách dùng mẫu này**
>
> - **Không sửa trực tiếp file này.** Sao chép thành file riêng trong `docs/reports/`:
>   - Vũ: `docs/reports/24120183_sysinfo.md`
>   - Thi: `docs/reports/24120452_vmprint.md`
>   - Sang: `docs/reports/24120428_pgaccess.md`
> - Ảnh minh chứng đặt trong `docs/reports/img/`, tên bắt đầu bằng MSSV.
> - Viết tiếng Việt, giữ thuật ngữ tiếng Anh khi cần (page table, PTE, free list, bitmask…).
> - Giảng viên yêu cầu báo cáo **ngắn gọn** ("short and sweet"), **không dán source code**. Mục tiêu: làm rõ lời giải, vấn đề gặp phải, phần chưa hoàn thành và thông tin giúp chấm điểm (kể cả điểm từng phần). Mỗi báo cáo cá nhân nên dài khoảng 2–4 trang khi xuất PDF.
> - Chỉ trích đoạn code rất ngắn (≤ 10 dòng) khi thật sự cần giải thích; ưu tiên mô tả luồng hoặc pseudo-code.
> - **Không bịa kết quả kiểm thử.** Chỉ ghi kết quả đã thực sự chạy, kèm log hoặc ảnh. Test chưa chạy thì ghi "Chưa chạy" và lý do.
> - Xóa toàn bộ các khối hướng dẫn `>` như khối này khi hoàn thành.
> - Sang tổng hợp ba báo cáo thành `24120183_24120428_24120452_Report.pdf`.

---

## 1. Thông tin thành viên và nhiệm vụ

| Mục | Nội dung |
|---|---|
| Họ tên | `<Họ và tên>` |
| MSSV | `<MSSV>` |
| Nhiệm vụ | `<Enhance Sysinfo / Print a page table / Detect which pages have been accessed>` |
| Điểm tối đa | `<2 / 4 / 4>` |
| Nhánh Git | `<feature/...>` |
| Commit cuối đã kiểm thử | `<hash ngắn>` |
| Mức độ hoàn thành (tự đánh giá) | `<Hoàn thành / Hoàn thành một phần — nêu rõ phần thiếu>` |

## 2. Mục tiêu và yêu cầu đề bài

> Tóm tắt **đúng** yêu cầu trong `Lab2_Memory.pdf` cho nhiệm vụ của bạn (3–6 gạch đầu dòng). Tách rõ:
> - **Yêu cầu của đề**.
> - **Quy ước/giả định của nhóm** (tham chiếu `TASK_ASSIGNMENT.md` §2), ví dụ: số hiệu syscall, giới hạn số trang, cơ chế bật in bảng trang.

- Yêu cầu của đề: …
- Quy ước/giả định của nhóm: …

## 3. Cơ sở lý thuyết liên quan

> Trình bày ngắn gọn **chỉ** phần lý thuyết dùng trực tiếp cho lời giải. Trích dẫn theo mẫu ở mục 13.
> - Sysinfo: free list, `struct run`, `kmem.lock`, luồng `kalloc`/`kfree`, `sbrk`.
> - Vmprint: Sv39, cây bảng trang 3 cấp, cấu trúc PTE (Hình 3.2), bố cục không gian địa chỉ tiến trình (Hình 3.4), `exec`.
> - Pgaccess: cấu trúc PTE, bit A theo RISC-V Privileged Spec, `walk()`, TLB/`sfence.vma`.

## 4. Khảo sát mã nguồn xv6 trước khi triển khai

> Liệt kê những gì đã có / chưa có tại baseline `lab2-baseline`, kèm vị trí `file:dòng`. Nêu khác biệt giữa sách và repo nếu có ảnh hưởng (xem `docs/xv6_chapter3.md`).

| Thành phần | Vị trí | Trạng thái tại baseline | Ghi chú |
|---|---|---|---|
| `<hàm/macro/struct>` | `kernel/<file>:<dòng>` | `<Có / Chưa có>` | `<dùng lại / cần thêm>` |

## 5. Phân tích và thiết kế giải pháp

> Trình bày:
> - Ý tưởng chính.
> - Các phương án đã cân nhắc và lý do chọn (ví dụ: duyệt free list hay dùng bộ đếm; cách bật in bảng trang; thứ tự `copyout` và xóa `PTE_A`).
> - Xử lý đồng bộ (lock nào, vì sao, có nguy cơ deadlock không).
> - Kiểm tra đối số và an toàn bộ nhớ.
> - Giả định khi đề mơ hồ.

- Ý tưởng chính: …
- Phương án cân nhắc và lựa chọn: …
- Đồng bộ: …
- Kiểm tra đầu vào / an toàn bộ nhớ: …
- Giả định: …

## 6. Luồng xử lý của chức năng

> Mô tả luồng từ user tới kernel và ngược lại bằng các bước đánh số hoặc sơ đồ văn bản. Không dán toàn bộ hàm.

```text
user: <lời gọi> → usys stub (li a7, SYS_<tên>; ecall)
  → uservec → usertrap → syscall() → sys_<tên>()
      1. …
      2. …
  → usertrapret → userret → user nhận giá trị trả về
```

## 7. Danh sách file đã chỉnh sửa và lý do

> Lấy từ `git diff lab2-baseline --stat` trên nhánh của bạn. Mỗi file một dòng; giải thích "vì sao cần" chứ không lặp lại code.

| File | Thay đổi (tóm tắt) | Lý do |
|---|---|---|
| `kernel/<file>` | … | … |
| `user/<file>` | … | … |
| `Makefile` | Thêm `<chương trình>` vào `UPROGS` | Đưa chương trình test vào `fs.img` |

## 8. Các trường hợp kiểm thử

> Thiết kế test **trước** khi xem kết quả. Bao gồm trường hợp bình thường, biên và đầu vào không hợp lệ. Có thể dùng lại ID trong `TASK_ASSIGNMENT.md` (F*, V*, P*). Cột "Kết quả thực tế" và "Trạng thái" điền ở mục 9 sau khi chạy.

| ID | Mục tiêu | Đầu vào / Kịch bản | Kết quả mong đợi | Kết quả thực tế | Trạng thái |
|---|---|---|---|---|---|
| `<X1>` | `<kiểm tra điều gì>` | `<lệnh / tham số>` | `<giá trị / hành vi>` | `<điền sau khi chạy>` | `<PASS / FAIL / Chưa chạy>` |
| `<X2>` | … | … | … | … | … |
| `R1` | Hồi quy Lab 1 | `sysinfotest` | `===== N/N PASS =====` | … | … |
| `R2` | Hồi quy Lab 1 | `filetest` | `filetest: ALL PASSED` | … | … |
| `R3` | Hồi quy xv6 | `forktest` | `fork test OK` | … | … |
| `R4` | Hồi quy xv6 | `usertests -q` | `ALL TESTS PASSED` | … | … |

## 9. Kết quả kiểm thử thực tế

> Môi trường: ghi rõ phiên bản để người chấm tái lập được.

| Mục | Giá trị |
|---|---|
| Hệ điều hành / WSL | `<Ubuntu xx.xx trên WSL2>` |
| Toolchain | `<riscv64-unknown-elf-gcc --version>` |
| QEMU | `<qemu-system-riscv64 --version>` |
| Commit kiểm thử | `<hash>` |

**Tổng hợp**

| Cấu hình | Test chức năng | Hồi quy Lab 1 | `usertests -q` |
|---|---|---|---|
| `CPUS=1` | `<x/y PASS>` | `<…>` | `<…>` |
| `CPUS=3` | `<x/y PASS>` | `<…>` | `<…>` |

**Minh chứng**

> Chèn ảnh hoặc trích log thật (rút gọn phần dài, giữ dòng tổng kết). Mỗi ảnh có chú thích.

![<Mô tả ngắn: lệnh, cấu hình CPU>](img/<MSSV>_<tên>.png)

*Hình 1. `<chú thích>`.*

```text
<dán đoạn log thật, ví dụ dòng tổng kết của chương trình test>
```

> **Riêng nhiệm vụ Print a page table (bắt buộc theo đề):** dán một output `vmprint` thật và lập bảng giải thích **mọi trang lá**:
>
> | Dòng output (idx L2/L1/L0) | VA của trang | PTE | Cờ (V R W X U) | PA | Nội dung logic (text / data / guard / stack / heap / trapframe / trampoline) | Giải thích quyền |
> |---|---|---|---|---|---|---|
>
> **Riêng nhiệm vụ Detect accessed pages:** ghi kết quả kiểm chứng bit A trên QEMU (phần cứng tự bật hay không) và bằng chứng sau khi xóa `PTE_A` thì truy cập mới vẫn được phát hiện.

## 10. Những khó khăn gặp phải và cách giải quyết

> Nêu vấn đề thật đã gặp (lỗi build, panic, kết quả sai, conflict…), nguyên nhân và cách khắc phục. Ghi cả hướng đã thử nhưng không thành công. Nếu môn học yêu cầu, ghi ngắn gọn việc sử dụng AI (dùng vào việc gì, đã kiểm chứng ra sao).

| Vấn đề | Nguyên nhân | Cách giải quyết |
|---|---|---|
| … | … | … |

## 11. Hạn chế còn tồn tại

> Trung thực về giới hạn: phần chưa làm, trường hợp chưa xử lý, giới hạn do thiết kế (ví dụ snapshot không nguyên tử, giới hạn số trang), hành vi phụ thuộc QEMU.

- …

## 12. Kết luận

> 3–5 câu: đã làm được gì, mức độ đáp ứng yêu cầu đề, điều rút ra về quản lý bộ nhớ trong xv6.

## 13. Tài liệu tham khảo

> Cách trích dẫn thống nhất:
> - Sách: `[xv6] R. Cox, F. Kaashoek, R. Morris, "xv6: a simple, Unix-like teaching operating system", 31/08/2024, mục 3.x, tr. yy.` Trong bài viết: `([xv6] §3.5, tr. 37)`.
> - Hình trong sách: `([xv6] Hình 3.2, tr. 33)`.
> - Mã nguồn: `kernel/vm.c:86` (`walk`). Số dòng tính tại `lab2-baseline` hoặc ghi rõ commit.
> - Spec: `[RISC-V Priv] The RISC-V Instruction Set Manual, Volume II: Privileged Architecture, mục <Sv39 / Sv32 page table entry>`.
> - Đề bài: `[Lab2] Lab2_Memory.pdf, mục "<tên mục>"`.
> - Không trích dẫn hoặc sao chép lời giải có sẵn trên mạng.

1. [Lab2] Đề bài Project 2 – Memory, `Lab2_Memory.pdf`.
2. [xv6] R. Cox, F. Kaashoek, R. Morris, *xv6: a simple, Unix-like teaching operating system*, bản 31/08/2024, Chương 3.
3. [RISC-V Priv] *The RISC-V Instruction Set Manual, Volume II: Privileged Architecture*.
4. Mã nguồn repository `xv6-lab2-group7`, tag `lab2-baseline` (commit `cd1e597`).
5. `<tài liệu khác nếu có>`
