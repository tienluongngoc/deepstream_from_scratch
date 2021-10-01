# Deep-stream-test-1

- Chương trình dựa trên 100% sorce code của NVIDIA Deepstream `/opt/nvidia/deepstream/deepstream-5.1/sources/apps/sample_apps/deepstream-test1/deepstream_test1_app.c`
- Chương trình sử dụng model detection gồm 4 class "Vehicle", "TwoWheeler", "Person", "Roadsign"

# Giải thích source code
src_code: `src/deepstream_test1/deepstream_test1.cpp`  

1. hàm osd_sink_pad_buffer_probe(): hàm xử lý các meta data của chương trình
2. hàm deepstream_test1(): 

- Dòng 107 - 115: Khai báo các elements gtreamer sử dụng trong deepstream  
    -   loop: Tạo vòng loop cho chương trình  
    -   pipeline: Tạo một pipe line để xử lý video  
    -   source: Element nhận đầu vào là luồng video *.h264  
    -   h264parser: Element dùng để pars luồng dữ liệu H264  
    -   decoder: decode dữ liệu  
    -   streammux: Gộp nhiều nguồn dữ liệu thành batch để xử lý  
    -   sink: Element (cuối pipeline) để nhận dữ liệu cho ra các thiết bị đầu cuối  
    -   pgie: Element của nvinfer, nơi xử lý model AI (nếu pipeline có nhiều model AI thì từ model thứ 2 được gọi là sgie)  
    -   nvvidconv: Convert dữ liệu hình ảnh (đọc thêm)  
    -   nvosd: Vẽ các thông tin lên bức ảnh: bbox, number of person ...  
    ...
- Dòng 128: Khởi tạo của gststreamer (luôn phải có)  
- Dòng 129: Tạo loop cho chương trình  
- Dòng 133: Tạo Pipeline cho chương trình  
- Dòng 136 - 180: Sử dụng câu lệnh gst_element_factory_make() để tạo ra các elements của gstreamer  
    - arg1: Loại element  
    - arg2: Tên cho element đó  
    - Tên của element là duy nhất trong chương trình (tránh trùng lặp)  
    - Lưu ý: Tạo element xong kiểm tra lại 
    - Dòng 167, 168: Do chạy môi trường trong docker nên thay thế `nveglglessink` bằng `fakesink` để chương trình không bị lỗi  
- Dòng 182 - 194: Set propreties cho element  
    - Đối với mối element sẽ có các properties ví dụ: source có `location`: là địa chỉ của source video...  
- Dòng 202 - 211: Thêm các element đã được tạo vào pipeline  
- Dòng 241 - 259: Link các element đã được thêm vào pipeline vào với nhau  
    - Hầu hết các elements đều được liên kết với nhau bằng phương pháp này, tuy nhiên đối với streammuxer được liên kết với các element trước nó theo một cơ chế khác dòng 213 - 236  
- Dòng 264 - 270: Khởi tạo 1 pad để đọc dữ liệu của pipeline từ buffer
    - Như đã đề cập ở phần xxx, mỗi element giao tiếp với element khác thông qua `pad` của chính nó, tại đây tạo 1 `pad` cho element `nvosd` và 1 hàm call_back để xử lý các dữ liệu trước khi vào `nvosd`
    - Chú ý: dòng 264 nếu thay thế `sink` bằng `src` thì hàm `osd_sink_pad_buffer_probe` sẽ xử lý dữ liệu sau khi đã qua `nvosd`
- Dòng 274: Thiết lập trạng thái cho pipeline, ở đây là `PLAYING`  
- Dòng 278: Tạo vòng loop cho pipeline, chương trình sẽ chạy đến khi báo lỗi hoăc hêt video  
- Dòng 280 - 287: Giải phóng các vùng nhớ đã khởi tạo  

# Giải thích config
Config: `apps/deepstream_test1/deepstream_test1_config.txt`
- net-scale-factor: Thông số để pre-process ảnh đầu vào [tham khảo](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvinfer.html). Để ý 1/255 = 0.0039215697906911373 dùng để normalize ảnh  
- model-engine-file: Đường dẫn file engine Tensorrt
- labelfile-path: Đường dẫn file chưa label của model  
- batch-size: Batch_size inference của model
- network-mode: Format dữ liệu khi inference Integer 0: FP32 1: INT8 2: FP16
- num-detected-classes: Số lượng class đối với model detection
- gie-unique-id: id của gie (element chưa model AI)

# BUG
- Lần đầu chạy chương trình comment dòng số 6 (model-engine-file) trong file `apps/deepstream_test1/deepstream_test1_config.txt`, để deepstream tự động sinh ra file engine TRT. Thực ra chương trình sẽ kiểm tra có tồn tại không, nếu không có file sẽ tự động sinh, nhưng code này đang bị lỗi mà mình chưa fix.

# Chạy chương trình

- Tạo container từ images của deepstream theo hướng dẫn tại `doc/create_container.md`  
- Cài đặt cmake để biên dịch chương trình C/C++ theo hướng dẫn: `doc/install_cmake.md`  
- Mở của số terminal
```
mkdir build
cd build
cmake ..
make
cd ..
./build/apps/deepstream_test1/deepstream_test1 /opt/nvidia/deepstream/deepstream-5.1/samples/streams/sample_720p.h264
```

# Làm sao để view kết quả
- Sử dụng máy có màn hình
- Ghi ra file
- Tạo luồng RTSP out
- Vẽ các kết quả lên ảnh rồi xem trên từng ảnh (Recommned, dễ dàng thực hiện so với 3 phương pháp trên)
