# AVOPSEMU PS2 v50 — CFC2 FBRST / khởi tạo VU

## Log và ảnh người dùng

avopsemu_ps2_log(3).txt và ảnh v49 cho thấy BIOS đã qua vòng khởi tạo DMAC và in Initialize VU1. Sau 15.917.021 lệnh, dừng tại PC 8000DF44, opcode 4842E000. Đây là CFC2 v0,FBRST (COP2 control index 28). Status.CU2 trong log bằng 1, nên lỗi là thiếu lệnh COP2 trong decoder, không phải quyền truy cập COP2 bị tắt.

PMODE vẫn 0, GIF QW=45, primitives=0. CPU dừng trước bước bật xuất hình. Sửa lệnh này chỉ đưa CPU qua điểm dừng đã biết, không đủ chứng minh toàn bộ BIOS sẽ có hình.

## Phần triển khai

- Thêm decoder CFC2/CTC2, QMFC2/QMTC2 và dạng interlock bit 0. Chỉ nhận encoding đúng; macro arithmetic/VCALL/BC2 chưa hỗ trợ vẫn dừng.
- Một bank 32 vector raw 128-bit của VU0, dùng cho các lệnh chuyển thanh ghi. VF0 giữ (0,0,0,1) theo bit float32; GPR0 và VI0 được bảo vệ.
- VI1..15 nhận 16-bit. CFC2 chuyển word có sign extension sang 64-bit dưới, giữ 64-bit trên GPR. QMFC2/QMTC2 chuyển nguyên 128-bit.
- Các control 16/17/18/20/21/22/26/27/28/29/31 được nhận diện. STATUS/CLIP/R/I/Q/CMSAR0 hiện chỉ là raw bootstrap latch; chưa có pipeline flag, MAC/FPU/VU arithmetic. MAC/TPC/VPU_STAT là read-only qua CTC2. P chỉ có trong micromode nên index 23 và các reserved control khác vẫn bị chặn.
- FBRST đọc lại mask 0C0C. Reset/force-break request bits được tiêu thụ, không giữ làm bit enable. Ghi reset/break hạ trạng thái busy của unit tương ứng, xóa VIF VEW đang mô hình hóa và ghi bộ đếm. Chưa mô phỏng thời gian, pipeline, IRQ reset/break hoặc toàn bộ chi tiết VU warm reset.
- FBRST control reset không xóa vector/data/microcode RAM hoặc VIF upload state. EE reset khởi tạo bank mới theo chính sách zero có thể tái hiện, VF0.w=1; bộ nhớ VU vẫn theo chính sách giữ RAM đã có của dự án.
- Status.CU2 tắt thì dừng rõ lỗi unusable, giống phạm vi kiểm tra coprocessor đang có; chưa chuyển trường hợp đó thành guest exception CE=2 hoàn chỉnh.
- Interlock khi VU0 idle dùng ngay dữ liệu hiện có. Nếu VU0 busy, báo cần VU execution; không giả lập hoàn tất. CTC2 CMSAR1 (kể cả giá trị zero) phải chạy vi chương trình VU1 nên vẫn báo chưa hỗ trợ, không lờ lệnh hoặc tự tạo kết quả.

Các trạng thái VU do đây cung cấp là giao tiếp thanh ghi và bootstrap control. Chưa có VU0/VU1 instruction engine, LQC2/SQC2, đồng bộ vi lệnh, XGKICK hoặc pipeline/latency.

## Kiểm thử

Fixture chạy đúng sáu opcode cuối: JAL ở 8000DA00, NOP delay, prologue tại 8000DF38/3C/40 và CFC2 tại 8000DF44. Stack đầu vào 80018E40, s0=7F, phần trên v0=53 theo log. Sau sáu lệnh: PC=8000DF48, stack/saved RA/s0 chính xác, v0.low=0, v0.high=53 được giữ, không first fault.

Negative regression cùng fixture: source EE v49 dừng opcode 4842E000, source v50 tới 8000DF48. Đây là đoạn lệnh tái hiện từ log, không phải ảnh BIOS đầy đủ.

Bộ COP2 mới kiểm tra bit pattern 128-bit, constant registers, sign extension, FBRST mask/reset hai unit, preserve VU memory/vector, read-only controls, CU2, idle/busy interlock và các lệnh reserved/launch không được hỗ trợ. Các bộ kiểm thử PS2 cũ tiếp tục chạy riêng, không liên kết PS1.

## Phạm vi bản cập nhật

Giữ cửa sổ GS tự mở khi nạp BIOS, decoder/renderer và bản vá DMA trước. Không đổi nhánh PS1. Chỉ đổi nhãn phiên bản PS2 và thêm phần chẩn đoán COP2/reset.

Chưa có BIOS thật, Windows SDK/MSVC hoặc Windows runtime trong môi trường này; không xác nhận BIOS vào menu hoặc native GUI bằng fixture. Sau khi build v50, nạp lại BIOS từ đầu rồi Run; nếu dừng tiếp, log sẽ có tên v50 và bộ đếm COP2/VU reset.

## Nguồn tham khảo

- PCSX2 VU0.cpp: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/VU0.cpp/ — CFC2/CTC2, QMFC2/QMTC2, FBRST mask/requests, CMSAR1 launch.
- PCSX2 VU.h: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/VU.h/ — tên/index control và P chỉ có ở micromode.
- EE Core Instruction Set Manual: https://docs.alexrp.com/mips/ee_insns.pdf — phân loại lệnh chuyển thanh ghi COP2.

Mã bổ sung được viết riêng dựa trên đối chiếu hành vi, không chép khối mã PCSX2. Một số chi tiết chỉ là mô hình bootstrap như ghi rõ ở trên.
