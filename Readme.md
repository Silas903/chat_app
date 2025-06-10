1. Kiến trúc tổng thể
Client <----(SSL/TLS over TCP Socket)----> Server
Client gửi/nhận dữ liệu đã mã hóa thông qua SSL socket.

Server xử lý nhiều client cùng lúc bằng threading.

Hệ thống hỗ trợ:

Đăng nhập/đăng ký

Nhắn tin riêng/tin nhắn nhóm

Ghi lịch sử chat (SQLite)

Xác thực tài khoản (file txt)

Bảo mật dữ liệu (OpenSSL)

📂 2. Cấu trúc thư mục
bash
Sao chép
Chỉnh sửa
chat-app/
├── server/
│   ├── main.c             # Entry point server
│   ├── server.h           # Định nghĩa hàm & struct
│   ├── ssl_utils.c/.h     # Hàm hỗ trợ SSL init, accept
│   └── chat_history.db    # SQLite DB lưu lịch sử chat
├── client/
│   ├── main.c             # Entry point client
│   └── ssl_utils.c/.h     # Tương tự server, tạo SSL connection tới server
├── accounts.json          # Lưu tài khoản người dùng
├── cert/
│   ├── server.crt         # Public certificate (x.509)
│   └── server.key         # Private key
├── Makefile               # Build script cho cả server và client
🧠 3. Các module chính
3.1. server/main.c
Khởi tạo SSL + TCP socket

Chấp nhận client và tạo thread/process cho từng client

Nhận message, kiểm tra lệnh:

LOGIN, REGISTER, GROUP, PRIVATE

Gọi broadcast() hoặc send_to_user()

Lưu lịch sử chat vào chat_history.db

3.2. server/ssl_utils.c
Tải certificate + private key

Tạo SSL context

Gắn SSL vào socket

Gọi SSL_accept để bắt tay

3.3. client/main.c
Kết nối đến server qua SSL

Menu đăng nhập/đăng ký

Gửi/nhận tin nhắn

Giao diện dòng lệnh chọn chế độ PRIVATE hoặc GROUP

3.4. accounts.json
Lưu thông tin tài khoản dưới dạng JSON:

{
  "alice": "hashed_pw",
  "bob": "hashed_pw"
}
🔒 4. Bảo mật
Sử dụng OpenSSL

SSL giao tiếp giữa client ↔ server

Không lưu mật khẩu thô — nên dùng hàm băm (SHA256/Bcrypt)

💬 5. Giao tiếp
Giao thức đơn giản:

LOGIN <username> <password>
REGISTER <username> <password>
GROUP <message>
PRIVATE <username> <message>
Client gửi chuỗi theo định dạng

Server phân tích và phản hồi theo logic

🧪 6. Makefile
Hỗ trợ:

bash
Sao chép
Chỉnh sửa
make server
make client
make clean
🧩 7. Extension (nâng cao nếu cần)
Thêm GUI (sau này)

Mã hóa tin nhắn lưu trữ

Lọc spam/từ khóa

Thêm thời gian vào lịch sử chat