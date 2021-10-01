CUDA_VER:=11.1
SRCFILES:=nvdsparsebbox_tiny_yolo.cpp
TARGET_LIB:=libnvdsinfer_custom_bbox_tiny_yolo.so
DEEPSTREAM_PATH:=/opt/nvidia/deepstream/deepstream-5.1

ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif
CC:= g++

CFLAGS:= -Wall -Werror -std=c++11 -shared -fPIC
CFLAGS+= -I$(DEEPSTREAM_PATH)/sources/includes -I/usr/local/cuda-$(CUDA_VER)/include

LIBS:= -lnvinfer -lnvparsers -L/usr/local/cuda-$(CUDA_VER)/lib64
LFLAGS:= -Wl,--start-group $(LIBS) -Wl,--end-group

all: $(TARGET_LIB)

$(TARGET_LIB) : $(SRCFILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf $(TARGET_LIB)
