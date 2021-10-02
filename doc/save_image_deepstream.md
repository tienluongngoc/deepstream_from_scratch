# Lưu hình ảnh trong deepstream
- Trong phần này mình sẽ hướng dẫn lưu hình trong deepstream thông qua xử lý metadata trong hàm `probe`
- Môi trường: Cần cài đặt thêm Opencv cho GPU  
- Chương trình deepstream dựa trên ví dụ [deepstream_test1](https://github.com/tienln4/deepstream_from_scratch/tree/main/src/deepstream_test1)  
- Dòng 49: Lấy thông tin từ surface
- Dòng 50 - 53: Lấy các thông tin về hình ảnh trong surface
    - `data_ptr`: Lưu địa chỉ ảnh stream trên GPU
    - src_height, src_width: Kích thước ảnh (Không giữ kích thước như đầu vào, kích thước này đúng bằng thông số cài đặt cho streammuxer)  
- Dòng 55: Tạo một ảnh trên GPU từ các thông tin trên 
- Dòng 56: Tải ảnh xuống CPU và lưu
- Dòng 65 - 73: Vẽ các bbox có `unique_component_id = 1`

# Chạy chương trình
- Tham khảo [ở đây](https://github.com/tienln4/deepstream_from_scratch/blob/main/doc/deepstream_test1.md)



