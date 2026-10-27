# AVOPSEMU PS2 v47 — CPU và BA0/DVE

## Lỗi trong log người dùng

`avopsemu_ps2_log.txt`: 15.817.748 lệnh, PC 80007CC0, opcode A4520000 (SH s2,0(v0)); v0=BA000010, s2=7E. First stop: unmapped/read-only write BA000010. TTY đã tới Initialize GS; GS có ghi SMODE/SYNC nhưng PMODE=0, GIF QW=0, không có primitive. Đây là thiếu MMIO BA0, không phải bằng chứng thiếu một thanh ghi GPR hay BIOS đã render menu.

Bộ tái hiện 10 lệnh từ 80007C9C đến 80007CC0 giữ nguyên opcode trong log và giá trị đầu vào. Nguồn BA0 cũ thất bại tại BA000010; nguồn mới tới 80007CC4, giữ mode 42/selector 7E và không ép PMODE hoặc hình ảnh.

## BA0/DVE

Mở rộng từ 8 halfword lên 18 halfword tại 1A000000..1A000022; alias KSEG0/KSEG1 như cũ. Chỉ nhận halfword căn chỉnh. Byte/word/doubleword/SQ và địa chỉ ngoài vùng vẫn bị từ chối.

Thay vòng đếm status 1/2/0 không phụ thuộc lệnh bằng mô hình lệnh có trạng thái. Mode 43 ghi vào DVE register được chọn; mode 42 đọc vào FIFO. Selector tại offset 10, FIFO bắt đầu 10/12, tối đa 10 halfword (ghi tối đa 9 dữ liệu sau selector). Có 256 latch DVE reset-zero; chưa biết ý nghĩa thực của từng thanh ghi, không dùng latch để tạo video GS.

Ready bit 1, error bit 0; chỉ lệnh hợp lệ có start bit 7 mới tiến triển. Status trả busy hai lần đọc rồi ready sticky; ghi offset 6 xóa ready và hủy tiến triển đang chờ. Độ trễ này là chính sách gần đúng, không đo từ phần cứng. Lệnh/độ dài không hỗ trợ báo error, không tràn bộ đệm. Lệnh đọc lặp giá trị của DVE register được chọn; thứ tự byte của giao thức DVE thực chưa được xác minh. Đây là mô hình chức năng có giới hạn, không phải toàn bộ DEV9/DVE/serial bus.

## CPU / COP0

Thêm bảy thanh ghi debug: BPC, IAB, IABM, DAB, DABM, DVB, DVBM qua selector 0,2..7 của COP0 register 24. Đọc sign-extend 32→64, giữ 64 bit trên của GPR. Hiện chỉ là latch đọc/ghi; chưa so khớp breakpoint, phát TRIG hoặc debug exception. Không tự tạo trạng thái breakpoint trúng.

Thêm PCCR/PCR0/PCR1 qua selector 0/1/3 của COP0 register 25. PCCR giữ các bit khả dụng, đếm theo CTE và chế độ kernel/supervisor/user/EXL; ERL ngăn đếm. Hỗ trợ event 1 (tick CPU mô hình) và 12 (lệnh hoàn tất). Không có timing dual issue, cache/TLB event hoặc performance-overflow exception. Các event khác giữ cấu hình nhưng không sinh số đếm. Counter wrap 32-bit là mô hình hiện tại, chưa phải hành vi ngoại lệ overflow đầy đủ. Reset đưa latch về zero để kết quả tái hiện được, không khẳng định mọi bit có reset phần cứng bằng zero.

Thêm ADD/SUB/DADD/DSUB/ADDI/DADDI có overflow, SYSCALL/BREAK và 12 trap register/immediate (TGE/TGEU/TLT/TLTU/TEQ/TNE và bản immediate). Khi xảy ra, cập nhật Cause/EPC/EXL, BD khi trong delay slot, chọn vector general theo BEV và cho guest chạy handler. Nested EXL giữ EPC/BD; ERET hiện có dùng lại trạng thái đó. Đích arithmetic không bị ghi khi overflow, kể cả GPR có phần trên cần giữ. Tính bằng unsigned trung gian để tránh signed overflow của C++.

Các opcode không hỗ trợ và lỗi ánh xạ bộ nhớ khác vẫn dừng kèm first fault; chưa triển khai đầy đủ mọi ngoại lệ, TLB translation hoặc cache chính xác. Đây là bổ sung trên CPU hiện có, không viết lại.

## Kiểm chứng và giới hạn

31 suite PS2 qua clean GCC build; 4 suite ASan/UBSan (BA0, CPU extension, GS boot trace, hardware) đạt. Bài SYNC cũ dùng BREAK làm opcode chưa hỗ trợ; nay đổi sentinel sang SPECIAL 05 dành riêng, vì BREAK đã được triển khai bằng ngoại lệ guest. Không bỏ kiểm tra strict unknown.

Nhánh PS1 đối chiếu từng byte với ZIP v46 và không thay đổi. SOURCE_MOVE_MANIFEST.json là snapshot lịch sử tách v46, không phải hash của mã CPU sau v47.

Không có BIOS thật, Windows SDK/MSVC hoặc thiết bị Windows để xác nhận native GUI hay BIOS hoàn tất. Chưa có IOP/VU execution đầy đủ, SPU2/CDVD, game PS2 hoặc menu BIOS. Cần test lại BIOS từ reset và xuất log v47 để thấy điểm kế tiếp.

## Nguồn tham khảo

- PCSX2 `Memory.cpp`, ba0W16/ba0R16: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/Memory.cpp — mẫu BA0/DVE, mode 42/43, cổng dữ liệu và ready/error. Mã tham khảo cũng ghi nhận phần chưa chính xác. Bản này giới hạn vùng và bố trí halfword rõ ràng, không sao chép vòng lặp byte-indexed của tham khảo.
- Sony EE Core User's Manual: https://docs.alexrp.com/mips/ee.pdf — nhóm debug/performance và ngoại lệ.
- Sony EE Core Instruction Set Manual: https://docs.alexrp.com/mips/ee_insns.pdf — phép toán có overflow, trap và lệnh chuyển thanh ghi.
- GNU binutils opcode table (upstream source mirror): https://chromium.googlesource.com/chromiumos/third_party/gdb/+/refs/heads/master/opcodes/mips-opc.c — encoding MFBPC/MFIAB/MFDAB/MFPS/MFPC và chiều ghi.
- PCSX2 `R5900.h`: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900.h — trường PCCR.

Mã bổ sung được viết tại dự án này dựa trên đối chiếu hành vi/encoding, không chép khối mã nguồn giả lập khác.
