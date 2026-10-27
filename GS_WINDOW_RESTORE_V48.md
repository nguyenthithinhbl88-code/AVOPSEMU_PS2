# v48 — khôi phục cửa sổ GS riêng khi nạp BIOS

Nạp BIOS thành công tự mở lại cửa sổ GS riêng, dùng cùng dữ liệu GS của CPU đang chạy. Cửa sổ có sẵn được hiện lại/khôi phục khi thu nhỏ, không tạo thêm phiên giả lập. Tiêu đề ghi loại chương trình và tên BIOS/ELF/demo để xác định đúng phiên. Đóng cửa sổ GS chỉ ẩn; nút Cửa sổ GS riêng mở lại nó. Màn hình tích hợp và debugger vẫn giữ.

Không đổi lõi CPU/GPU hoặc cách boot. F5 chạy/tạm dừng trong cửa sổ chính; cửa sổ GS riêng theo dõi đầu ra cùng phiên. Gói không có BIOS. Chưa kiểm tra native Windows; không chạy lại bộ kiểm thử lõi vì chỉ sửa thao tác cửa sổ và tiêu đề.
