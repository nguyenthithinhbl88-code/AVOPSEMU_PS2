# AVOPSEMU PS2 v48 — giao diện và khởi tạo DMAC

## Log mới

avopsemu_ps2_log(1).txt xác nhận v47 đi qua lỗi BA000010 trước đó. TTY đã ghi Initialize GS, INTC, TIMER và DMAC. Dừng ở 15.915.975 lệnh, PC 8000DE14, opcode AC400080 (SW zero,0x80(v0)), v0=B0008000. Địa chỉ đích B0008080 tương ứng DMA0 SADR.

PS2SDK có dma_sadr[0/1/2] tại 10008080/10009080/1000A080 và ghi zero khi khởi tạo kênh. Lõi trước chỉ cho SADR trên kênh 8/9; nay thêm 0/1/2 theo cùng đường đọc/ghi cấu hình. Giữ mask 3FF0, alias KSEG0/KSEG1, kiểm tra active DMA và giới hạn word. Không mở SADR cho kênh 3..7, không biến toàn bộ trang MMIO thành RAM.

Đây là hỗ trợ thanh ghi khởi tạo/cấu hình. Chưa bổ sung hành vi truyền scratchpad mới cho VIF/GIF hoặc engine DMA kênh 8/9. Cơ chế truyền đang có không thay đổi.

Bài tái hiện dùng 14 opcode trong phần đuôi log, stack ban đầu và bảng địa chỉ DMA0 tại RAM 80015A78. Nguồn hardware cũ dừng B0008080; nguồn mới tới 8000DE18. Chỉ xác nhận đoạn này, chưa biết toàn bộ bước sau của BIOS vì không có ảnh BIOS ở môi trường test. Không ép thanh ghi GS, cờ IOP hoặc vẽ menu giả.

## EE / GS Studio

- Thanh công cụ sáu nút lớn: Nạp BIOS, Mở ELF, Chạy/Tạm dừng, Dừng, Reset, Lưu log.
- Sidebar tên tệp, loại chương trình, Chạy nhanh, cửa sổ GS riêng và gợi ý phím.
- Màn hình GS mặc định tích hợp trong cửa sổ, vẫn có cửa sổ riêng dùng cùng framebuffer.
- Khi PMODE chưa bật mạch xuất hình, hiện trạng thái 'GS chưa bật xuất hình'; không gán đây là BIOS menu hoặc kết quả render.
- Debugger mặc định ẩn, F9 bật/tắt khu vực bên phải. Không định dạng toàn bộ diagnostic khi debugger ẩn. Status/MIPS làm mới khoảng 250 ms; không gọi MIPS là FPS.
- F5 Chạy/Tạm dừng, F6 Dừng, F9 Debugger. Hỗ trợ Tab qua nút bằng IsDialogMessage. Menu demo/step/batch vẫn giữ từ nhánh cũ.
- Bật/tắt debugger, mở cửa sổ GS và chuyển mức chạy nhanh không tự dừng CPU; lưu log và thao tác thay chương trình sẽ tạm dừng.
- Chạy nhanh vẫn là ngân sách UI 12 ms/lượt, thường 4 ms/lượt, giới hạn 250.000 lệnh; chưa chuyển sang worker và chưa hứa tăng tốc thực tế.
- Reset đặt CPU về entry hiện có và giữ RAM như bản trước; không phải cold-reset toàn bộ máy.
- Nút vẽ riêng, nền đệm, font Segoe UI, màu xanh đậm, icon và version metadata PS2 v48; không phụ thuộc tệp của PS1 khi build.

## Kiểm chứng

33 suite PS2 đạt bằng clean GCC C++17. Bộ layout mới kiểm tra 48 tổ hợp kích thước/debugger, phát hiện và sửa thanh công cụ quá rộng ở cỡ nhỏ. Bốn suite ASan/UBSan đạt: DMA init, hardware, GS boot trace, UI layout.

Không có MSVC/Windows SDK hoặc Windows runtime; chưa xác nhận build resource/compiler Win32, giao diện thực tế, focus/Tab, resizing, mở/đóng cửa sổ GS trên máy Windows. Kiểm thử layout chỉ xác nhận tọa độ, không thay thế kiểm tra native UI.

Nhánh PS1 đối chiếu byte với gói đã giao và không đổi. Mã PS2 v47 CPU/debug/performance/BA0 được giữ, chỉ đổi nhãn diagnostic thành v48 và bổ sung điều kiện SADR.

## Build / thử lại

Mở riêng AVOPSEMU_PS2 trong Visual Studio. Chọn windows-release, Startup Item AVOPSEMU_PS2.exe. Dùng thư mục build mới nếu còn cache từ dự án gộp. Nạp BIOS → Chạy. Nếu dừng, bấm Lưu log rồi gửi log có tiêu đề v48.

Nguồn chính: PS2SDK ee/dma/src/dma.c, bảng dma_sadr và dma_channel_initialize: https://github.com/ps2dev/ps2sdk/blob/master/ee/dma/src/dma.c . Mã thay đổi được viết trong dự án, không sao chép khối nguồn SDK.
