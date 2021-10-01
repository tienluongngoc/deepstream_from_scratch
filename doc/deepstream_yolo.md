First of all, I would like to thank [thatbrguy](https://github.com/thatbrguy/Deep-Stream-ONNX). this source code was heavily based on his repo

# Tải model weight 
- Ví dụ này thực hiện yolov2 Deepstream
- [Tải model tiny YOLO V2](https://github.com/onnx/models/blob/master/vision/object_detection_segmentation/tiny-yolov2/model/tinyyolov2-7.onnx)
# Plugin để post-process 
- Đổi với mỗi model output của engine là khác nhau, một plugin được viết riêng cho mỗi model để thực hiện post-process (nms...)  
- Việc cần thiết và quan trọng khi thực deploy một model bằng deepstream là viết plugin này và thường gọi là `custom_bbox_parser`  
- Src_code plugin: `./src/tiny_yolo2_deepstream/custom_bbox_parser/nvdsparsebbox_tiny_yolo.cpp`
```
NvDsInferParseCustomYoloV2Tiny(
    std::vector<NvDsInferLayerInfo> const& outputLayersInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferParseObjectInfo>& objectList);
```
- Trong đó:
    - outputLayersInfo: Là một vector chứa kết quả model sau khi inference
    - networkInfo: Thông tin về model, ví dụ: input_shape...
    - detectionParams: Một số thông tin config
    - objectList: Chứa thông tin sau khi post-process cụ thể: bbox, class_id, score...
- Việc quan trọng là xử lý `outputLayersInfo` để thu được thông tin detection và đẩy vào `objectList`. Sau đó, deepstream sẽ tự động tạo các metadata ứng với thông tin trong `objectList`

# Giải thích source code
- Ví dụ này hoàn toàn giống với  [deepstream_test1](doc/deepstream_test1.md), chỉ thay đổi duy nhất model AI, nên do đó sẽ thay đổi plugin để post-process model. Trong ví dụ trước sử dụng plugin mặc địch của deepstream viết cho model đó.
# Chạy chương trình
- build custom_bbox_parser
```
cd ./src/tiny_yolo2_deepstream/custom_bbox_parser
make
```
- Chạy chương trình
```
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
./build/apps/tiny_yolo2_deepstream/tiny_yolo2_deepstream /opt/nvidia/deepstream/deepstream-5.1/samples/streams/sample_720p.h264
```