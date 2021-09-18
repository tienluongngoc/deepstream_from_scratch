# Cài đặt môi trường
- Để thuận tiện mình sẽ hướng dẫn chạy các ví dụ Deepstream trong Docker  
- [Tải Deepstream image](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_docker_containers.html)  
  Có 5 bản image khác nhau phù hợp với mục đích sử dụng, với bản deepstream:5.1-21.02-triton đầy đủ
```
$ docker pull nvcr.io/nvidia/deepstream:5.1-21.02-triton
$ docker run -it --rm --gpus all --network host nvcr.io/nvidia/deepstream:5.1-21.02-triton /bin/bash
```
- Source code và các thư viện của deepstream ở ``/opt/nvidia/deepstream/deepstream-5.1``

# Chạy demo deepstream-test1
- deepstream-test1: Là một ví dụ đơn giản của bài toán object detection trên Deepstream, với 4 class "Vehicle , RoadSign, TwoWheeler,
Person"
- Vào thư mục chứa source_code
```
$ cd /opt/nvidia/deepstream/deepstream-5.1/sources/apps/sample_apps/deepstream-test1
$ deepstream-test1-app ../../../../samples/streams/sample_720p.h264 
```
- Trong lần chạy đầu tiên chúng ta thấy chương trình chỉ in ra màn hình `Frame Number = 1 Number of objects = 6 Vehicle Count = 4 Person Count = 2` một lần và báo lỗi. Nguyên nhân là chương trình sử dụng `element nveglglessink` (sẽ nói rõ ở phần sau) dùng để show lên màn hình nhưng hiện tại chúng ta đang ở trong container.
- Để giải quyết lỗi thực hiện như sau
```
$ vim deepstream_test1_app.c
```
- Tìm đến dòng code `sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");` thay thế `nveglglessink` bằng `fakesink` và lưu file
- Build lại chương trình
```
CUDA_VER=11.1 make install
CUDA_VER=11.1 make clear
```
- Chạy lại chương trình và không còn lỗi :)))
```
$ deepstream-test1-app ../../../../samples/streams/sample_720p.h264 
```
# Chạy demo với deepstream-app 
```
$ deepstream-app -c ../../../../samples/configs/deepstream-app/source30_1080p_dec_infer-resnet_tiled_display_int8.txt 
```
- Tương tự như ví dụ trên chương trình sẽ báo lỗi `** ERROR: <main:675>: Could not open X Display` là do không có thiết bị để hiển thị (hiểu nôm na như thế)
- Để giải quyết bug này thì chúng ta sửa file config của chương trình như sau
```
$ vim /opt/nvidia/deepstream/deepstream-5.1/samples/configs/deepstream-app/source30_1080p_dec_infer-resnet_tiled_display_int8.txt 
```
```
[sink0]
enable=1
#Type - 1=FakeSink 2=EglSink 3=File
type=2
sync=1
source-id=0
gpu-id=0
nvbuf-memory-type=0
```
- Sửa enable=0
- - Chạy lại chương trình và thấy các thông tin in ra màn hình là FPS của trên từng nguồn
- Với config `source30_1080p_dec_infer-resnet_tiled_display_int8.txt`: chạy chương trình object detection với 30 video cùng lúc
- Các bạn có thể thử với những config khác
- Trong phần này mình đã hướng dẫn chạy 2 ví dụ Deepstream, để hiểu rõ các ví dụ khác đọc thêm [tại đây](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_C_Sample_Apps.html)