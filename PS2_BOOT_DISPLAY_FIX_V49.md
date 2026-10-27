# AVOPSEMU PS2 v49 — vòng khởi tạo DMA và theo dõi xuất hình

## Đọc log thật

avopsemu_ps2_log(2).txt: 15.916.017 lệnh, PC 8000DE14, opcode AC400080, v0=B000B000, v1=4, a0=80015A84, t1=31F. First stop là ghi B000B080, thuộc kênh DMA3/IPU_FROM.

PMODE=0, cả DISPFB/DISPLAY1/2 bằng zero; GIF đã nhận 45 quadword, 1 packet nhưng primitives/pixels=0. Đây là trạng thái chưa bật mạch xuất hình và chưa có hình vẽ để scanout tại lúc CPU dừng. Log không chứng minh renderer đang bỏ mất một menu đã dựng; cũng không chứng minh sửa DMAC là đủ để hoàn tất BIOS.

## Lỗi gốc của vòng khởi tạo

Firmware dùng chung bố cục để xóa các offset +80, +00, +30, +10, +50, +40 cho các kênh được chọn bằng mặt nạ 31F. V48 mới mở SADR cho DMA0/1/2 và 8/9, nên đến kênh 3 lại dừng. Các ASR ở +40/+50 trên những kênh không có source-chain stack cũng gây lỗi tiếp theo nếu chỉ mở thêm một địa chỉ.

V49 giữ các thanh ghi DMA thực hiện có. Riêng ô chưa có chức năng trong bố cục xóa dùng backing riêng, word 32-bit:

- Kênh 3..9: +40/+50, 14 ô.
- Kênh 3..7: +80, 5 ô.

Tổng 19 ô compatibility, không gọi chúng là ASR/SADR chức năng trên phần cứng. Ghi/đọc lại giữ nguyên 32-bit trong backing; không start/complete DMA, không đổi QWC, không phát IRQ. Ghi khi CHCR.STR đang bật vẫn bị chặn. Byte/halfword/doubleword/SQ, các lỗ +60/+70/+84/+90 và địa chỉ ngoài danh sách vẫn lỗi. Reset xóa backing và bộ đếm.

Chính sách này đối chiếu đường xử lý default/backing của PCSX2 HwWrite/HwRead, nhưng được giới hạn vào đúng các ô chung mà firmware khởi tạo. Chưa xác minh đọc lại ô reserved trên PS2 thật; đó là chính sách tương thích, không thêm khả năng IPU/SIF/SPR DMA.

## Kiểm thử trọn vòng

Fixture dùng toàn bộ 20 opcode của vòng lặp có trong log, mặt nạ 31F và bảng 10 base DMA chuẩn, chạy từ 8000DDE8 tới 8000DE38. Các kênh 0/1/2/3/4/8/9 được xóa; các kênh bỏ qua giữ giá trị đã nạp. Không dừng sau riêng lệnh SW B000B080.

So sánh cùng fixture: mã hardware v48 dừng B000B080; v49 thoát vòng tại 8000DE38. Đây là tái hiện đoạn lệnh, không phải chạy đầy đủ BIOS Sony. Bảng DMA trong fixture là dữ liệu kiểm thử theo bản đồ chuẩn, không trích toàn bộ RAM/BIOS người dùng.

## Theo dõi GS

Ghi lịch sử 32 lần ghi MMIO GS thành công gần nhất: PC ngữ cảnh, instruction counter, địa chỉ, độ rộng, giá trị. Chỉ thêm sau khi GS chấp nhận lệnh; ghi lỗi không được báo là thành công. Diagnostic không có tác dụng đọc/ghi MMIO. Ngữ cảnh có thể là CPU hoặc thiết lập từ host/demo; chưa gắn cờ phân biệt nguồn.

Boot status ghi riêng enable, DISPFB và DISPLAY của mỗi mạch; ghi rõ CPU first stop nếu có. Cửa sổ GS giữ nguyên cơ chế riêng, tự mở khi nạp BIOS. Khi PMODE tắt và CPU có lỗi, thông báo 'CPU đã dừng trước khi GS bật xuất hình' cùng hướng dẫn Lưu log; khi chưa có lỗi chỉ báo GS chưa bật xuất hình. Không ép PMODE, không tự tạo drawing packet hoặc giao diện PS2.

## Giới hạn

Chưa có Windows SDK/MSVC, ảnh BIOS thật hoặc game để xác nhận GUI native hoặc BIOS tới Browser/System Configuration. IOP/VU execution, SPU2/CDVD và TLB translation đầy đủ còn thiếu như trước. Sửa lỗi làm CPU vượt qua đoạn khởi tạo trong log, không khẳng định đã giải quyết mọi nguyên nhân màn hình đen sau đó.

## Nguồn tra cứu

- PCSX2 HwWrite.cpp: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwWrite.cpp/ — DMAC dispatch và default 32-bit backing assignment.
- PCSX2 HwRead.cpp: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp/ — default đọc backing.
- PCSX2 Dmac.h: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Dmac.h/ — bank layout, mapping kênh.
- Tài liệu phần cứng do tác giả ps2tek biên soạn: https://psi-rockin.github.io/ps2tek/ — bản đồ DMA, GS/GIF.

Mã bổ sung viết riêng, không sao chép khối mã giả lập khác. Tài liệu cũ về guard DMA3..7 là lịch sử; chính sách v49 ở trên thay thế phần guard +40/+50/+80 tương ứng.
