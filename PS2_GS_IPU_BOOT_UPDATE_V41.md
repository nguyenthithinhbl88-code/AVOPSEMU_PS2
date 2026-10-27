# v41 — PS2 GS/IPU bootstrap update

Log mới chạy 15813796 lệnh, đã qua BA0, dừng tại PC `8000D6E0`:
`90430000` = LBU v1,0(v0), v0=`FFFFFFFFBF80146E`.
Console còn ở `Initialize GS ...`; PMODE=0, GIF QW=0, primitives=0.
Chưa có bằng chứng BIOS đã dựng hình.

## Các thay đổi

| Khối | Hành vi v41 | Giới hạn |
|---|---|---|
| IOP probe `1F80146E` | Byte read trả 0 qua ba alias, đếm lượt đọc | Phản hồi reset-zero thử nghiệm; chưa biết ý nghĩa phần cứng, không giả board ID/IOP-ready |
| 19 thanh ghi GS privileged đã có | Thêm byte/halfword lane, giữ word/doubleword; căn chỉnh và khoảng trống vẫn kiểm tra | Không mở tràn cả vùng MMIO |
| GS CSR | Mô hình ID=55h, REV=1Bh, FIFO empty; ID/REV chỉ đọc; W1C theo lane | Revision mô hình theo tham chiếu PCSX2, không tự dò chip thật |
| GS CSR RESET | Reset trạng thái vẽ/transfer, CSR/IMR; giữ VRAM, PCRTC, GIF packet parser và số liệu chẩn đoán | Chưa có GS FIFO/timing chính xác |
| IPU CMD/CTRL/BP/TOP | Đọc trạng thái khởi tạo, ghi cấu hình và reset CTRL | Chưa có bitstream/FIFO/DMA/MPEG |
| IPU BCLR/SETTH | BCLR đặt BP 7-bit; SETTH giữ hai trường threshold 9-bit; hoàn tất đồng bộ, INTC IRQ8 | Không hỗ trợ lệnh giải mã còn lại |

Không còn xóa 4 MiB VRAM mỗi lần phần mềm ghi CSR RESET; cold reset/load
vẫn giữ chính sách xóa bộ nhớ của core. CSR FLUSH không có hàng đợi vẽ cần
chờ trong renderer đồng bộ hiện tại. Không ép PMODE hoặc tự tạo menu BIOS.

IPU: CMD/TOP đọc bằng word/doubleword, CTRL/BP bằng word. BP/TOP chỉ đọc;
CMD upper word đọc zero. CTRL chỉ nhận các bit cấu hình, IDP=3 được chuyển
về 1 theo chính sách tham chiếu. Reset CTRL xóa control/BP, giữ SETTH
thresholds như tham chiếu. BCLR/SETTH hoàn tất ngay; ngắt được xóa bằng
INTC_STAT W1C bình thường. Threshold 9-bit theo ps2tek; PCSX2 1.6 chỉ giữ
8-bit, vì vậy đây không phải sao chép hành vi SETTH của PCSX2.
CMD/TOP giữ zero khởi tạo trong tập chức năng này; chưa mô phỏng thiếu dữ
liệu/busy của quá trình đọc bitstream. Lệnh MPEG/FDEC/VDEC không được trả
thành công giả: báo lỗi rõ ràng trước khi sửa trạng thái/ngắt.

Probe 146E: chỉ mở byte read đang có trong log. Width khác, write và byte
bên cạnh vẫn lỗi. Tham khảo IOP read handler PCSX2 trả backing byte ở địa
chỉ chưa nhận diện, backing được xóa khi reset. PSLab chọn zero chỉ cho
đúng probe này; không ánh xạ toàn bộ IOP thành bộ nhớ giả.

## Kiểm tra đoạn BIOS trong log

Phát lại đúng 14 lệnh từ `8000D6AC` đến `8000D6E0`, kiểm tra GIF reset,
CSR reset, SYNC, ghi zero xuống `80022618`, rồi LBU qua probe. Kết quả:
PC=`8000D6E4`, v1=0, không lỗi, không tự phát sinh gói vẽ/PMODE.
Đây là replay phần đuôi với trạng thái đầu được khởi tạo, không phải chạy
lại toàn bộ 15.8 triệu lệnh của Sony BIOS. Ngoài ra có fixture EE tự viết
reset/cấu hình IPU, BCLR và đọc BP qua lệnh CPU thật.

## Chạy thử

Build lại Release x64 trong Visual Studio 2022, mở pslab.exe v41.
Tools > PS2 EE laboratory > Load BIOS > bật Fast run > Run.
Không cần bấm Run 100K lặp lại. Nếu dừng, Save log mới phải ghi
`PSLab v41 PS2`; log mới cho biết điểm tiếp theo thực sự cần sửa.

Bản sửa PS1 GT2 v39 và bàn phím v40 được giữ lại. Core/GUI PS2 vẫn thiếu
IOP/VU execution, MMU/TLB translation đầy đủ, SPU2/CDVD và renderer hoàn
chỉnh. Chưa xác nhận giao diện BIOS PS2 thật hoặc game PS2. Không có BIOS/
game Sony trong môi trường kiểm thử; không có MSVC/Windows SDK/runtime.

## Nguồn tham khảo

- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwRead.cpp — default page1 byte backing reads.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/IopHw.cpp — reset clears IOP register backing.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/GS.h — CSR reset identity/FIFO model.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/GS.cpp — narrow CSR writes, masks and soft-reset scope.
- https://psi-rockin.github.io/ps2tek/ — IPU registers, command fields and threshold widths.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/IPU/IPU.cpp — BCLR/SETTH immediate IRQ, CTRL masks/reset.

Mã triển khai mới viết trong dự án; không chép tệp nguồn của emulator khác.
