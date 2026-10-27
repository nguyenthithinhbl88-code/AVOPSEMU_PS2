# AVOPSEMU PS2 v50 — dự án độc lập

Bản v50 bổ sung giao tiếp thanh ghi COP2, sửa CFC2 FBRST tại 8000DF44 khi BIOS khởi tạo VU1. Xem `PS2_COP2_BOOT_FIX_V50.md`; chưa có bộ thực thi vi lệnh VU hoặc xác nhận menu BIOS.

Bản v49 trước đó sửa toàn bộ vòng xóa cấu hình DMA trong log (2), thêm 19 backing slot compatibility và lịch sử ghi GS. Giữ cửa sổ GS riêng tự mở khi nạp BIOS. Xem `PS2_BOOT_DISPLAY_FIX_V49.md`. Chưa xác nhận BIOS tới menu.

Bản v48 trước đó sửa DMA0/1/2 SADR trong log mới và thay giao diện PS2 bằng EE / GS Studio. Màn hình GS tích hợp, menu tiếng Việt, debugger F9, F5 chạy/tạm dừng, F6 dừng. Xem `PS2_UI_DMA_UPDATE_V48.md`.

Bản v47 trước đó sửa dừng SH BA000010, mở rộng BA0/DVE, thêm nhóm debug/performance COP0 và ngoại lệ SYSCALL/BREAK/TRAP/overflow. Xem `PS2_CPU_BA0_UPDATE_V47.md` và `VALIDATION_V47.txt`. Đây chưa phải bản boot BIOS tới menu được xác nhận.

Đây là nhánh PS2 được **chuyển từ PSLab/AVOPSEMU hiện có**, không viết lại lõi. Ở thời điểm tách v46, các tệp EE, COP0/COP1, MMI, DMA, GIF, GS, IOP config, VIF, IPU, BA0 và CACHE giữ nguyên từng byte. Từ v47, EE/BA0 được cập nhật tiếp trên nền mã đó. `SOURCE_MOVE_MANIFEST.json` ghi SHA-256 trước/sau cho 58 tệp chuyển; 56 không thay đổi. Hai tệp window chỉ chỉnh tích hợp để chạy/thoát độc lập, thêm entrypoint Win32 mới.

## Visual Studio 2022

1. Cài **Desktop development with C++** và **C++ CMake tools for Windows**.
2. File → Open → Folder → chọn thư mục **AVOPSEMU_PS2** có CMakeLists.txt.
3. Chọn preset **windows-release**, Startup Item **AVOPSEMU_PS2.exe**, Ctrl+F5.
4. **Nạp BIOS** chọn BIOS PS2 của bạn → **Chạy (F5)**. Nạp BIOS thành công tự mở cửa sổ GS riêng để theo dõi. Màn hình GS cũng nằm sẵn trong cửa sổ chính; **Hiển thị → Mở cửa sổ GS riêng** mở thêm khung riêng.
5. Các demo CPU/GIF/GPU/DMA/chain và Open ELF giữ nguyên từ nhánh cũ. **Lưu log** xuất chẩn đoán PS2. Các demo nằm trong menu **Công cụ**.

Hoặc Developer PowerShell:

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release --target avopsemu_ps2
.\build-vs\Release\AVOPSEMU_PS2.exe
```

Solution mới: `build-vs/AVOPSEMU_PS2.sln`. Không cần mã nguồn, thư viện hoặc EXE của dự án PS1. Sao chép riêng thư mục này sang vị trí khác vẫn đủ nguồn để build.

CLI giữ tên `pslab_ps2` để giữ tương thích:

```powershell
cmake --build build-vs --config Release --target pslab_ps2
.\build-vs\Release\pslab_ps2.exe --gpu-demo --steps 1000
```

## Phạm vi hiện tại

Kế thừa nguyên trạng khả năng v41–v45: EE subset, thanh ghi/config, DMA/GIF, renderer GS cơ bản, debug BIOS. Chưa có thực thi IOP/VU đầy đủ, MMU/TLB đầy đủ, SPU2/CDVD hoặc chạy game PS2. Chưa xác nhận boot BIOS thật tới giao diện PS2. Tách dự án không tự bổ sung các phần này và không ép dựng menu giả.

Các ELF/BIN demo trong examples là mã kiểm thử tự viết, không phải BIOS Sony. Không kèm BIOS/game thương mại.

Kiểm thử:

```powershell
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
```

Các tài liệu lịch sử giữ nguyên tên cũ và có thể nhắc tới cửa sổ PS2 trong PS1; từ v46, chạy trực tiếp AVOPSEMU_PS2.exe theo hướng dẫn trên. Môi trường kiểm thử hiện tại là Linux/GCC, không có Windows SDK/MSVC.
