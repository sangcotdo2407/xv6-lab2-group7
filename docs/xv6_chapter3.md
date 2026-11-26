# xv6 Chương 3 — Page tables: tài liệu tra cứu cho coding agents

**Nguồn:** R. Cox, F. Kaashoek, R. Morris, *xv6: a simple, Unix-like teaching operating system*, bản 31/08/2024, Chương 3 (tr. 31–42). Đối chiếu với mã nguồn repo tại tag `lab2-baseline` (commit `cd1e597`).

**Nhãn nguồn gốc** (bắt buộc đọc để không nhầm sách với suy luận):

| Nhãn | Ý nghĩa |
|---|---|
| **[Sách]** | Nội dung tóm lược từ sách. |
| **[Mã nguồn]** | Kiểm tra trực tiếp trong repo; số dòng tại `lab2-baseline`. |
| **[Spec]** | Theo RISC-V Privileged Architecture Manual, không có trong lời văn sách. |
| **[Suy luận]** | Suy ra từ code/sách, chưa có trong sách, cần kiểm chứng khi dùng. |
| **[Đề]** | Lấy từ `Lab2_Memory.pdf`. |

Tài liệu này **không** chứa lời giải Lab 2. Chỉ giải thích khái niệm và chỉ ra vị trí code liên quan.

---

## Bản đồ nhanh

| Mục | Trang | Nội dung | Sysinfo (Vũ) | Vmprint (Thi) | Pgaccess (Sang) |
|---|---|---|---|---|---|
| Mở đầu | 31 | Vai trò của page table | · | · | · |
| 3.1 Paging hardware | 31–34 | Sv39, PTE, cây 3 cấp, TLB, `satp` (Hình 3.1, 3.2) | | ★ | ★ |
| 3.2 Kernel address space | 34–35 | Direct map, trampoline, kstack, guard page (Hình 3.3) | · | ☆ | |
| 3.3 Code: creating an address space | 35–37 | `walk`, `mappages`, `kvm*`, `sfence.vma` | | ★ | ★ |
| 3.4 Physical memory allocation | 37 | Free list ý tưởng | ★ | | |
| 3.5 Code: Physical memory allocator | 37–38 | `kalloc.c` | ★ | | |
| 3.6 Process address space | 38–39 | Bố cục user, quyền, guard page (Hình 3.4) | ☆ | ★ | ★ |
| 3.7 Code: sbrk | 39–40 | `growproc`, `uvmalloc`, `uvmdealloc` | ★ | | ★ |
| 3.8 Code: exec | 40–41 | ELF, dựng bảng trang mới, stack | ☆ | ★ | ☆ |
| 3.9 Real world | 41–42 | So sánh hệ thống thật | | | |
| 3.10 Exercises | 42 | Bài tập | | ☆ (bài 2) | |

★ bắt buộc · ☆ nên đọc · · đọc lướt

---

## Khác biệt giữa sách (2024) và repo (xv6-labs-2023 + Lab 1)

| Điểm | Sách | Repo | Ảnh hưởng |
|---|---|---|---|
| Số dòng `vm.c` (`kvmmake` 20, `kvminit` 54, `kvminithart` 62, `walk` 86, `walkaddr` 109, `kvmmap` 132, `mappages` 144, `uvmunmap` 178, `uvmalloc` 233, `uvmcopy` 313, `copyout` 359, `copyinstr` 415) | như liệt kê | **khớp** [Mã nguồn] | Tra theo sách được |
| Số dòng `kalloc.c` (`struct run` 17, `kmem` 21–24, `kinit` 27, `kfree` 47, `kalloc` 69) | như liệt kê | **khớp** | — |
| Số dòng `exec.c` (`loadseg` prototype 10, `exec` 23, `namei` 36, `proc_pagetable` 49, `uvmalloc` 65, commit 125, free old 129) | như liệt kê | **khớp** | — |
| `MAXVA` | `riscv.h:375`/`378` | `riscv.h:363` | Lệch dòng |
| `growproc` | `proc.c:260` | `proc.c:263` (khác phiên bản xv6 và có thay đổi của Lab 1) | Lệch dòng |
| `sfence.vma` khi về user | `trampoline.S:89` | `userret` ở `trampoline.S:108–110`; dòng 89 thuộc `uservec` (vào kernel) | Dùng số dòng repo |
| Timer (Ch. 5) | `stimecmp` | CLINT + machine mode (`memlayout.h` có `CLINT_MTIMECMP`) | Không ảnh hưởng Ch. 3 |
| `kalloc` lấp trang bằng giá trị 5 | không nêu | có (`kalloc.c:80`) | Chỉ để bắt lỗi dùng bộ nhớ chưa khởi tạo |
| `PTE_A`, `vmprint`, `pgaccess`, `user/pgtbltest.c` | — | **chưa có** | Thuộc Lab 2 |

---

## Mở đầu chương (tr. 31) [Sách]

- Page table là cơ chế giúp hệ điều hành cấp cho mỗi tiến trình một không gian địa chỉ và bộ nhớ riêng; quyết định một địa chỉ có nghĩa là gì và phần bộ nhớ vật lý nào được truy cập.
- xv6 dùng page table để cô lập tiến trình và ghép nhiều không gian địa chỉ lên một bộ nhớ vật lý.
- Hai "mẹo" xv6 dùng:
  - ánh xạ cùng một trang (trampoline) vào nhiều không gian địa chỉ;
  - bảo vệ stack của kernel và user bằng một trang không được ánh xạ (guard page).

---

## 3.1 Paging hardware (tr. 31–34)

### Nội dung chính [Sách]

- Mọi lệnh RISC-V (user lẫn kernel) dùng **địa chỉ ảo (VA)**; RAM được đánh **địa chỉ vật lý (PA)**. Phần cứng page table dịch VA → PA.
- xv6 chạy **Sv39**:
  - chỉ dùng 39 bit thấp của VA 64 bit;
  - 25 bit cao không dùng cho dịch địa chỉ.
- Về logic, page table là mảng 2^27 PTE. Mỗi PTE gồm **PPN 44 bit** + các cờ.
- Dịch địa chỉ:
  - 27 bit cao trong 39 bit làm chỉ số để tìm PTE;
  - PA 56 bit = 44 bit PPN (từ PTE) + 12 bit offset (chép từ VA).
- Đơn vị ánh xạ là khối 4096 = 2^12 byte, gọi là **trang (page)**.
- 2^39 byte = 512 GB không gian ảo; 2^56 byte không gian vật lý. PTE còn chỗ cho PPN tăng thêm 10 bit. Sv48 dùng VA 48 bit.
- **Cây 3 cấp**:
  - gốc là một page-table page 4096 byte chứa 512 PTE trỏ tới các page-table page cấp tiếp theo;
  - 9 bit cao nhất của 27 bit chọn PTE ở trang gốc, 9 bit giữa chọn ở cấp kế, 9 bit thấp chọn PTE cuối.
- Nếu một trong ba PTE cần cho việc dịch không có mặt, phần cứng sinh **page-fault exception** để kernel xử lý (Chương 4).
- Cây 3 cấp tiết kiệm bộ nhớ. Ví dụ: chương trình chỉ dùng vài trang từ địa chỉ 0 → entry 1..511 của trang gốc không hợp lệ, tiết kiệm 511 trang thư mục trung gian và 511 × 512 trang thư mục cấp cuối.
- Nhược điểm: phải nạp 3 PTE cho mỗi lần dịch. CPU cache PTE trong **TLB (Translation Look-aside Buffer)**.
- Cờ PTE:
  - `PTE_V`: PTE có mặt; nếu không có, mọi tham chiếu gây exception;
  - `PTE_R`: được đọc;
  - `PTE_W`: được ghi;
  - `PTE_X`: được thực thi;
  - `PTE_U`: được truy cập ở user mode (không có `PTE_U` thì chỉ supervisor mode dùng được).
- Kernel ghi PA của trang gốc vào thanh ghi **`satp`**. Mỗi CPU có `satp` riêng, nên các CPU có thể chạy tiến trình khác nhau.
- Thuật ngữ:
  - *physical memory*: ô nhớ trong RAM;
  - *address space*: tập VA hợp lệ trong một page table;
  - *user memory*: address space user + bộ nhớ vật lý mà page table cho phép truy cập;
  - *virtual memory*: các ý tưởng/kỹ thuật quản lý page table.

### Hình 3.1 (tr. 32) — góc nhìn logic một cấp [Sách]

```text
VA (64 bit):  | EXT 25 | Index 27 | Offset 12 |
                          │
                          ▼  bảng 2^27 phần tử, mỗi phần tử: | PPN 44 | Flags 10 |
PA (56 bit):  | PPN 44 (lấy từ PTE) | Offset 12 (chép từ VA) |
```

### Hình 3.2 (tr. 33) — dịch thật 3 cấp và cấu trúc PTE [Sách]

```text
VA:  | EXT 25 | L2 9 | L1 9 | L0 9 | Offset 12 |
satp ──► page directory (gốc) [L2] ──PPN──► page directory [L1] ──PPN──► page directory [L0] ──PPN──► PA = PPN|Offset
```

Cấu trúc một PTE 64 bit (vị trí bit theo [Spec]; tên bit theo Hình 3.2):

| Bit | Tên | Ý nghĩa |
|---|---|---|
| 63–54 | Reserved | Dành riêng |
| 53–10 | PPN | Physical Page Number (44 bit) |
| 9–8 | RSW | Dành cho phần mềm supervisor |
| 7 | D | Dirty (Hình 3.2 ghi: "0 in page directory") |
| 6 | **A** | **Accessed** — bit dùng cho `pgaccess` |
| 5 | G | Global |
| 4 | U | User |
| 3 | X | Executable |
| 2 | W | Writable |
| 1 | R | Readable |
| 0 | V | Valid |

- [Spec] A được đặt khi trang được đọc, ghi hoặc thực thi (qua page walker) kể từ lần A bị xóa gần nhất; D được đặt khi trang được ghi. Spec cho phép hiện thực **hoặc** để phần cứng tự cập nhật A/D, **hoặc** sinh page fault để phần mềm cập nhật.
- [Spec] Với PTE không phải lá, A/D/U là dành riêng, phần mềm phải để 0.
- [Spec] PTE hợp lệ có R = W = X = 0 là PTE **trỏ tới cấp dưới**; ngược lại là PTE **lá** (tổ hợp W = 1, R = 0 là dành riêng).
- [Suy luận] `mappages()` (`vm.c:165`) không bao giờ đặt A/D. Nếu QEMU sinh page fault khi A = 0 thì xv6 không thể boot. Baseline boot được, nên QEMU của nhóm tự cập nhật A/D. Kiểm chứng thực nghiệm theo TASK_ASSIGNMENT.md §6, mục "Quy trình kiểm chứng bit A trên QEMU".

### Liên hệ mã nguồn [Mã nguồn]

| Khái niệm | Vị trí |
|---|---|
| `pte_t` (uint64), `pagetable_t` (uint64*, 512 PTE) | `kernel/riscv.h:330–331` |
| `PGSIZE`, `PGSHIFT`, `PGROUNDUP`, `PGROUNDDOWN` | `kernel/riscv.h:335–339` |
| `PTE_V/R/W/X/U` (bit 0–4); **chưa có `PTE_A`** | `kernel/riscv.h:341–345` |
| `PA2PTE(pa) = (pa >> 12) << 10`; `PTE2PA(pte) = (pte >> 10) << 12`; `PTE_FLAGS(pte) = pte & 0x3FF` | `kernel/riscv.h:348–352` |
| `PX(level, va) = (va >> (12 + 9*level)) & 0x1FF` | `kernel/riscv.h:355–357` |
| `MAXVA = 1L << 38` (ít hơn Sv39 một bit để tránh mở rộng dấu) | `kernel/riscv.h:359–363` |
| `MAKE_SATP`, `w_satp`, `sfence_vma` | `kernel/riscv.h` |

[Suy luận] Quy tắc phân biệt lá/không lá trong code: `freewalk()` (`vm.c:284`) coi PTE `PTE_V` mà không có R/W/X là trỏ xuống cấp dưới.

---

## 3.2 Kernel address space (tr. 34–35)

### Nội dung chính [Sách]

- xv6 có **một page table cho mỗi tiến trình** (không gian user) và **một page table cho kernel**.
- QEMU mô phỏng RAM từ PA `0x80000000` tới ít nhất `0x88000000` (`PHYSTOP`). Thiết bị I/O được memory-mapped dưới `0x80000000`.
- Kernel dùng **direct mapping** (VA = PA) cho RAM và thanh ghi thiết bị. Ví dụ kernel nằm ở `KERNBASE = 0x80000000` cả ở VA lẫn PA. Lợi ích: `fork` dùng PA do allocator trả về như một VA để chép bộ nhớ.
- Hai loại VA kernel **không** direct-map:
  - **Trampoline**: ánh xạ ở đỉnh không gian ảo (user page table có cùng ánh xạ). Trang vật lý trampoline xuất hiện hai lần trong không gian kernel: ở đỉnh và ở vị trí direct-map.
  - **Kernel stack** của mỗi tiến trình: ánh xạ cao, bên dưới là **guard page** có PTE không hợp lệ (`PTE_V` = 0). Tràn stack sẽ gây exception và panic, thay vì âm thầm ghi đè bộ nhớ kernel.
- Quyền:
  - trampoline và kernel text: `PTE_R | PTE_X`;
  - các trang khác: `PTE_R | PTE_W`;
  - guard page: không hợp lệ.

### Hình 3.3 (tr. 34) [Sách]

```text
VA kernel (trái)                            PA (phải)
MAXVA ┌ Trampoline        R-X  ───────────► (trang trampoline trong kernel text)
      │ Guard page        ---
      │ Kstack 0          RW-  ───────────► (một trang RAM)
      │ Guard page        ---
      │ Kstack 1 ...      RW-
PHYSTOP (0x88000000) ┌ Free memory  RW-  ═► RAM (direct map)
                     │ Kernel data  RW-  ═►
KERNBASE(0x80000000) └ Kernel text  R-X  ═►
0x10001000  VIRTIO disk RW-  ═► VIRTIO disk
0x10000000  UART0       RW-  ═► UART0
0x0C000000  PLIC        RW-  ═► PLIC
                                 (PA còn có CLINT 0x02000000, boot ROM 0x1000)
```

### Liên hệ mã nguồn [Mã nguồn]

- Hằng số: `kernel/memlayout.h` gồm `UART0`, `VIRTIO0`, `PLIC`, `KERNBASE`, `PHYSTOP = KERNBASE + 128 MB`, `TRAMPOLINE = MAXVA - PGSIZE`, `KSTACK(p) = TRAMPOLINE - (p+1)*2*PGSIZE`, `TRAPFRAME = TRAMPOLINE - PGSIZE`.
- `kvmmake()` (`vm.c:20–50`) ánh xạ UART0, VIRTIO0, PLIC, kernel text (R|X), kernel data + RAM tới `PHYSTOP` (R|W), trampoline (R|X), rồi `proc_mapstacks()` (`proc.c:33`).
- [Mã nguồn] Bản repo **không** ánh xạ CLINT vào kernel page table (timer xử lý ở machine mode).

---

## 3.3 Code: creating an address space (tr. 35–37)

### Nội dung chính [Sách]

- Mã quản lý address space nằm ở `vm.c`. Kiểu trung tâm là `pagetable_t`: con trỏ tới trang gốc, có thể là page table của kernel hoặc của một tiến trình.
- Hàm trung tâm:
  - `walk`: tìm PTE cho một VA;
  - `mappages`: cài PTE cho ánh xạ mới.
- Quy ước tên: `kvm*` thao tác page table kernel; `uvm*` thao tác page table user. `copyout`/`copyin` chép dữ liệu tới/từ VA user (nằm ở `vm.c` vì phải tự dịch địa chỉ).
- Khởi động:
  - `main` gọi `kvminit` → `kvmmake` **trước khi bật paging**, nên địa chỉ lúc đó là địa chỉ vật lý;
  - `kvmmake` cấp trang gốc rồi gọi `kvmmap` để cài các ánh xạ;
  - `proc_mapstacks` cấp kernel stack cho mỗi tiến trình, ánh xạ tại `KSTACK`.
- `kvmmap` → `mappages`: với mỗi trang trong khoảng, gọi `walk` lấy địa chỉ PTE rồi ghi PPN, quyền (`PTE_W`, `PTE_X`, `PTE_R`) và `PTE_V` (`vm.c:165`).
- `walk` (`vm.c:86`) mô phỏng phần cứng:
  - đi xuống từng cấp bằng 9 bit tương ứng của VA;
  - nếu PTE ở cấp 2 hoặc 1 không hợp lệ và `alloc` ≠ 0, cấp một page-table page mới và ghi PA của nó vào PTE;
  - trả về địa chỉ PTE ở cấp thấp nhất (`vm.c:102`).
- Cơ chế này **phụ thuộc direct map**: `walk` lấy PA của cấp dưới từ PTE (`vm.c:94`) rồi dùng PA đó như VA để đọc PTE kế tiếp (`vm.c:92`).
- `kvminithart` (`vm.c:62`) ghi PA trang gốc vào `satp`. Từ đó CPU dịch bằng page table kernel; nhờ direct map, lệnh kế tiếp vẫn đúng địa chỉ.
- TLB:
  - khi đổi page table, kernel phải vô hiệu hóa TLB, nếu không TLB có thể dùng ánh xạ cũ trỏ tới trang đã cấp cho tiến trình khác;
  - lệnh `sfence.vma` flush TLB của CPU hiện tại; xv6 thực thi nó trong `kvminithart` sau khi nạp `satp` và trong trampoline trước khi về user;
  - cũng cần `sfence.vma` **trước** khi đổi `satp` để chờ các load/store trước đó hoàn tất.
- RISC-V có ASID để flush một phần; **xv6 không dùng**.

### Liên hệ mã nguồn và gợi ý cho Lab 2

- [Mã nguồn] `walk()` gọi `panic("walk")` nếu `va >= MAXVA` (`vm.c:88–89`). Mọi VA đến từ user phải được kiểm tra trước khi đưa vào `walk`.
- [Mã nguồn] `walk(pt, va, 0)` trả `0` nếu thiếu page-table page trung gian. PTE trả về có thể chưa hợp lệ, nên vẫn phải kiểm tra `PTE_V`.
- [Mã nguồn] `freewalk()` (`vm.c:279–294`) duyệt đệ quy 512 PTE mỗi cấp, là khuôn mẫu đề gợi ý cho `vmprint` [Đề].
- [Mã nguồn] `sfence.vma` khi về user nằm trong `userret` (`trampoline.S:108–110`). [Suy luận] Mỗi lần trở về user, TLB bị flush; sau khi kernel sửa PTE (ví dụ xóa `PTE_A`), lần truy cập user kế tiếp sẽ đi qua page walker.

---

## 3.4 Physical memory allocation (tr. 37) [Sách]

- Kernel cấp/giải phóng bộ nhớ vật lý lúc chạy cho page table, bộ nhớ user, kernel stack và pipe buffer.
- Vùng dùng để cấp phát: từ **cuối kernel** tới `PHYSTOP`.
- Đơn vị: **cả trang 4096 byte**.
- Theo dõi trang trống bằng **danh sách liên kết luồn qua chính các trang trống**:
  - cấp phát = gỡ một trang khỏi danh sách;
  - giải phóng = thêm trang vào danh sách.

---

## 3.5 Code: Physical memory allocator (tr. 37–38)

### Nội dung chính [Sách]

- Allocator ở `kalloc.c`. Cấu trúc dữ liệu là **free list**; mỗi phần tử là `struct run` (`kalloc.c:17`) được lưu **ngay trong trang trống** đó.
- Free list được bảo vệ bởi **spin lock**; list và lock được gói trong một struct (`kalloc.c:21–24`) để thể hiện lock bảo vệ các trường trong struct. Chi tiết lock ở Chương 6.
- `main` gọi `kinit` (`kalloc.c:27`) để đưa mọi trang từ cuối kernel tới `PHYSTOP` vào free list. xv6 **giả định RAM 128 MB** thay vì đọc cấu hình phần cứng.
- `kinit` → `freerange` → `kfree` từng trang. PTE chỉ trỏ được tới PA thẳng trang, nên `freerange` dùng `PGROUNDUP`.
- Allocator lúc thì coi địa chỉ là số nguyên (để tính toán), lúc là con trỏ (để đọc/ghi `struct run`), nên có nhiều ép kiểu.
- `kfree` (`kalloc.c:47`):
  - ghi toàn trang bằng giá trị 1 để code dùng con trỏ treo (dangling reference) đọc phải rác và lỗi sớm hơn;
  - đẩy trang vào **đầu** danh sách.
- `kalloc` gỡ và trả phần tử đầu danh sách.

### Liên hệ mã nguồn [Mã nguồn]

| Thành phần | Vị trí | Ghi chú |
|---|---|---|
| `extern char end[]` | `kalloc.c:14` | Địa chỉ đầu tiên sau kernel, do `kernel.ld` đặt |
| `struct { struct spinlock lock; struct run *freelist; } kmem` | `kalloc.c:21–24` | Lock tên `"kmem"` |
| `kfree` kiểm tra PA thẳng trang và trong `[end, PHYSTOP)`, nếu sai thì `panic("kfree")` | `kalloc.c:51–52` | |
| `kfree` giữ `kmem.lock` khi chèn vào đầu list | `kalloc.c:59–62` | |
| `kalloc` giữ `kmem.lock` khi gỡ đầu list; trả `0` nếu hết bộ nhớ; lấp trang bằng 5 | `kalloc.c:73–81` | |
| **Không có** biến đếm số trang trống | — | Lab 2 (Vũ) |

- [Suy luận] `freerange` gọi `kfree` từ địa chỉ thấp lên cao, mỗi trang được đẩy vào đầu list. Vì vậy sau `kinit`, đầu list là trang **cao nhất** (gần `PHYSTOP`), giải thích vì sao PA trong ví dụ của đề có dạng `0x87f6....`.
- [Suy luận] Muốn biết bộ nhớ trống phải đọc free list (hoặc một bộ đếm) **khi giữ `kmem.lock`**, vì CPU khác có thể đồng thời gọi `kalloc`/`kfree`.

---

## 3.6 Process address space (tr. 38–39)

### Nội dung chính [Sách]

- Mỗi tiến trình có page table riêng; khi chuyển tiến trình, xv6 đổi page table.
- Bộ nhớ user bắt đầu từ VA 0 và có thể lớn tới `MAXVA` (về nguyên tắc 256 GB).
- Quyền theo vùng:
  - **text**: `PTE_R | PTE_X | PTE_U`;
  - **data, stack, heap**: `PTE_R | PTE_W | PTE_U`.
- Lý do hardening:
  - text không có `PTE_W`: ghi nhầm vào con trỏ NULL (sửa lệnh ở địa chỉ 0) gây page fault ngay, kernel giết tiến trình và in thông báo;
  - data không có `PTE_X`: không thể vô tình nhảy vào data để thực thi;
  - trong thực tế, phân quyền cẩn thận và ngẫu nhiên hóa bố cục còn giúp chống tấn công khai thác lỗi.
- **Stack là một trang**, nội dung ban đầu do `exec` tạo:
  - các chuỗi đối số và mảng con trỏ tới chúng nằm ở đỉnh;
  - ngay dưới là các giá trị để `main(argc, argv)` chạy như vừa được gọi.
- **Guard page** ngay dưới stack bằng cách **xóa `PTE_U`**: stack tràn xuống sẽ gây page fault. Hệ thống thật có thể tự cấp thêm stack.
- Khi tiến trình xin thêm bộ nhớ:
  - xv6 dùng `kalloc` rồi thêm PTE với `PTE_W | PTE_R | PTE_U | PTE_V`;
  - PTE không dùng có `PTE_V` = 0.
- Ba ví dụ hay của page table:
  - mỗi tiến trình có bộ nhớ riêng;
  - VA liên tục từ 0 dù PA không liên tục;
  - trampoline ánh xạ ở đỉnh mọi không gian user **không có `PTE_U`**, nên chỉ kernel dùng được.

### Hình 3.4 (tr. 39) — không gian địa chỉ user và stack ban đầu [Sách]

```text
MAXVA ┌ trampoline   R X - -   (không U)
      │ trapframe    R - W -   (không U)
      │ unused
      │ heap         R - W U
      │ stack        R - W U   (PAGESIZE)  ── đỉnh stack: argument 0..N, 0,
      │ guard page             (U bị xóa)        địa chỉ argv[argc]..argv[0],
      │ data         R - W U                     địa chỉ của argv (đối số của main)
      │ unused      (Page aligned)
0     └ text         R X - U
```

### Liên hệ mã nguồn [Mã nguồn]

- `proc_pagetable()` (`proc.c:180–209`) tạo page table rỗng rồi ánh xạ:
  - `TRAMPOLINE` → `trampoline` với `PTE_R | PTE_X`;
  - `TRAPFRAME` → `p->trapframe` với `PTE_R | PTE_W`;
  - cả hai **không có `PTE_U`**.
- `uvmalloc()` (`vm.c:233`) luôn thêm `PTE_R | PTE_U | xperm`.
- `exec` dùng `flags2perm()` (`exec.c:12–20`) đổi cờ ELF thành `PTE_X`/`PTE_W`.
- Guard page: `uvmclear()` (`vm.c:345–353`) xóa `PTE_U`.
- [Suy luận] Với `TRAMPOLINE = 2^38 - 4096`: `PX(2) = 255`, `PX(1) = 511`, `PX(0) = 511`; `TRAPFRAME` có `PX(0) = 510`. Vì vậy output `vmprint` luôn có nhánh `..255 → .. ..511 → .. .. ..510/511`.

### Giải mã ví dụ output trong đề [Đề] + [Suy luận]

| Dòng (đề) | Cờ thấp (hex) | Bit | Diễn giải |
|---|---|---|---|
| `..0: pte 0x…9c01` | `0x01` | V | Trỏ xuống cấp dưới (không R/W/X) |
| `.. .. ..0: pte 0x…a01b` | `0x1b` | V R X U | Text |
| `.. .. ..1: pte 0x…9417` | `0x17` | V R W U | Data |
| `.. .. ..2: pte 0x…9007` | `0x07` | V R W (không U) | Guard page |
| `.. .. ..3: pte 0x…8c17` | `0x17` | V R W U | Stack |
| `.. .. ..509: pte 0x…cc13` | `0x13` | V R U | Trang `USYSCALL` của bài "speed up system calls" (MIT). **Không có trong repo này** |
| `.. .. ..510: pte 0x…d007` | `0x07` | V R W | Trapframe |
| `.. .. ..511: pte 0x…1c0b` | `0x0b` | V R X | Trampoline (PA `0x80007000` nằm trong kernel text) |

Lưu ý [Đề]: lời văn của đề nói cấp cuối "has entries 0, 1, and 2 mapped" nhưng ví dụ in 0–3. Output thật phụ thuộc kích thước chương trình.

---

## 3.7 Code: sbrk (tr. 39–40)

### Nội dung chính [Sách]

- `sbrk` là system call để tiến trình tăng hoặc giảm bộ nhớ, cài bằng `growproc`. `growproc` gọi `uvmalloc` hoặc `uvmdealloc` tùy `n` dương hay âm.
- `uvmalloc` (`vm.c:233`) cấp bộ nhớ vật lý bằng `kalloc`, **xóa về 0**, rồi thêm PTE bằng `mappages`.
- `uvmdealloc` gọi `uvmunmap` (`vm.c:178`); hàm này dùng `walk` tìm PTE và `kfree` bộ nhớ vật lý.
- Page table của tiến trình là **bản ghi duy nhất** về các trang vật lý đã cấp cho nó, vì vậy giải phóng bộ nhớ user phải duyệt page table.

### Liên hệ mã nguồn [Mã nguồn]

- `sys_sbrk` (`sysproc.c:41–51`) → `growproc` (`proc.c:263–278`), `uvmalloc(..., PTE_W)` khi tăng.
- `uvmalloc` cấp **ngay** (eager): không có lazy allocation trong repo.
- `mappages` → `walk(..., 1)` có thể cấp thêm page-table page.
- `uvmunmap` chỉ giải phóng **trang lá**. Page-table page trung gian chỉ được giải phóng ở `freewalk` (`vm.c:279`) khi gọi `uvmfree` (tiến trình thoát hoặc `exec` bỏ ảnh cũ).

[Suy luận] dùng cho kiểm thử:

- Sau `sbrk(+N trang)`, bộ nhớ trống giảm **ít nhất** N trang (có thể thêm vài page-table page).
- Sau `sbrk(-N trang)`, bộ nhớ trống tăng **đúng** N trang.
- `uvmalloc` ghi trang qua PA (direct map), không qua page table user. Kernel làm vậy không bật `PTE_A` của trang user; chỉ truy cập từ user mode qua page walker mới bật.

---

## 3.8 Code: exec (tr. 40–41)

### Nội dung chính [Sách]

- `exec` thay không gian địa chỉ user bằng dữ liệu đọc từ file thực thi ELF.
- Trình tự:
  1. `exec` (`exec.c:23`) mở file bằng `namei` (`exec.c:36`), đọc ELF header `struct elfhdr` (`elf.h:6`), rồi các program section header `struct proghdr` (`elf.h:25`). Chương trình xv6 có hai: một cho lệnh, một cho dữ liệu.
  2. Kiểm tra nhanh magic number `ELF_MAGIC` (`elf.h:3`); nếu đúng thì coi file là ELF hợp lệ.
  3. Tạo page table mới chưa có ánh xạ user bằng `proc_pagetable` (`exec.c:49`).
  4. Cấp bộ nhớ cho mỗi segment bằng `uvmalloc` (`exec.c:65`), nạp nội dung bằng `loadseg` (`exec.c:10`). `loadseg` dùng `walkaddr` tìm PA và `readi` đọc file.
- Ví dụ `objdump -p user/_init`:
  - segment text nạp ở VA 0 với quyền `r-x`;
  - segment data ở `0x1000` với quyền `rw-`, `filesz 0x10`, `memsz 0x30`;
  - `filesz < memsz` nghĩa là phần thiếu được lấp 0 (biến toàn cục C).
- **Stack**:
  - cấp đúng một trang stack;
  - chép từng chuỗi đối số lên đỉnh, ghi con trỏ vào `ustack`, đặt con trỏ null cuối `argv`;
  - `argc` trả qua giá trị trả về của system call (`a0`), `argv` qua `a1` trong trapframe.
- Một **trang không truy cập được** đặt ngay dưới stack:
  - chương trình dùng quá một trang stack sẽ fault;
  - đối số quá lớn làm `copyout` (`vm.c:359`) trả `-1`.
- Khi lỗi (ví dụ segment không hợp lệ), nhảy tới `bad`, giải phóng ảnh mới, trả `-1`. **Chỉ giải phóng ảnh cũ khi chắc chắn thành công**: commit page table mới (`exec.c:125`) rồi giải phóng cái cũ (`exec.c:129`).
- An ninh:
  - địa chỉ trong file ELF do người dùng kiểm soát, nên `exec` kiểm tra, ví dụ `if(ph.vaddr + ph.memsz < ph.vaddr)` để chống tràn số 64 bit;
  - trong xv6 RISC-V, `loadseg` nạp vào page table của tiến trình chứ không phải của kernel, nên không thể ghi vào kernel.

### Liên hệ mã nguồn [Mã nguồn]

| Bước | Vị trí |
|---|---|
| Kiểm tra `ph.memsz < ph.filesz`, tràn số, `ph.vaddr % PGSIZE` | `exec.c:58–63` |
| Cấp 2 trang (guard + stack), `uvmclear` trang dưới | `exec.c:81–88` |
| Đẩy đối số, `argv[]` | `exec.c:91–110` |
| Commit: `p->pagetable = pagetable; p->sz = sz;` | `exec.c:124–129` |
| `return argc;` (vị trí đề gợi ý gọi `vmprint`) | `exec.c:131` |
| Nhãn `bad:` | `exec.c:133–140` |
| `sys_exec` chép đường dẫn/đối số (mỗi đối số một trang `kalloc`) rồi gọi `exec` | `sysfile.c:440–480` |

[Suy luận] Sau dòng 125–129, `p->pagetable` là bảng mới; trước đó vẫn là bảng cũ. Lời gọi in bảng trang phải đặt sau commit, và đường `bad:` không được in.

---

## 3.9 Real world (tr. 41–42) [Sách]

- Hệ điều hành thật kết hợp paging với page-fault exception phức tạp hơn nhiều (Chương 4).
- xv6 được đơn giản hóa nhờ direct map và giả định RAM ở `0x80000000`. Điều này đúng trên QEMU nhưng không thực tế trên phần cứng thật; kernel nghiêm túc dùng page table để biến bố cục vật lý tùy ý thành bố cục ảo dự đoán được.
- RISC-V hỗ trợ bảo vệ ở mức địa chỉ vật lý, xv6 không dùng.
- **Super page**:
  - trang nhỏ hợp lý khi RAM nhỏ (cấp phát và page-out mịn);
  - trang lớn hợp lý khi RAM lớn và giảm chi phí thao tác page table.
- xv6 không có bộ cấp phát kiểu `malloc` cho đối tượng nhỏ trong kernel, chỉ cấp trang 4096 byte. Kernel thật cần cả cấp phát nhỏ lẫn lớn.
- Cấp phát bộ nhớ là chủ đề nóng; ngày nay người ta quan tâm tốc độ hơn hiệu quả không gian.

---

## 3.10 Exercises (tr. 42) [Sách]

1. Đọc device tree của RISC-V để biết dung lượng bộ nhớ vật lý.
2. Viết chương trình gọi `sbrk(1)`, quan sát page table trước và sau: kernel cấp bao nhiêu? PTE của vùng nhớ mới chứa gì? *(Liên quan trực tiếp tới vmprint và sysinfo.)*
3. Dùng super page cho kernel.
4. Hỗ trợ script `#!` trong `exec`.
5. Ngẫu nhiên hóa bố cục không gian địa chỉ (ASLR) cho kernel.

Các bài tập này **không** thuộc yêu cầu chấm của Lab 2.

---

## Phụ lục A — Phần ngoài Chương 3 có liên quan (chỉ trỏ tới, không tóm lược)

| Mục sách | Trang | Dùng khi |
|---|---|---|
| 4.3 Code: Calling system calls | 47 | Đăng ký syscall mới (`syscalls[]`, `a7`, `usys.pl`) |
| 4.4 Code: System call arguments | 47–48 | `argint`, `argaddr`, `copyout`/`copyin` an toàn với con trỏ user |
| 4.6 Page-fault exceptions | 48–51 | Hiểu vì sao truy cập trang không hợp lệ làm tiến trình bị giết |
| 6.3 Code: Using locks | 64 | `kmem.lock` bảo vệ free list |
| 6.6 Locks and interrupt handlers | 67 | Spinlock tắt ngắt; không giữ lâu |

## Phụ lục B — Bảng tra nhanh theo nhiệm vụ

| Câu hỏi | Xem |
|---|---|
| Đếm bộ nhớ trống ở đâu, cần lock gì? | §3.5, `kalloc.c:21–24` |
| Vì sao `sbrk(-n)` không trả đủ bộ nhớ như lúc tăng? | §3.7 (page-table page không được giải phóng) |
| PTE nào là lá, PTE nào trỏ xuống? | §3.1 (bảng bit), `freewalk` `vm.c:284` |
| Tại sao có nhánh 255/511/510–511? | §3.6 (giải mã `TRAMPOLINE`/`TRAPFRAME`) |
| Gọi `vmprint` ở đâu để không ảnh hưởng `exec`? | §3.8 (sau commit, trước `return argc`) |
| Bit A nằm ở đâu, ai bật? | §3.1 (bit 6, [Spec]), §3.3 (TLB) |
| Vì sao kernel ghi vào trang user không bật bit A? | §3.2 (direct map), §3.7 [Suy luận] |
| `walk` có thể panic không? | §3.3 (`va >= MAXVA`) |
