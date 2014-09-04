ifneq ($(BUILD_TINY_ANDROID),true)
LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
include $(CLEAR_VARS)

LOCAL_MODULE := sensors_native_hal

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
ifeq ($(TARGET_BOARD_PLATFORM),msm8960)
  LOCAL_CFLAGS += -DTARGET_8930
endif

LOCAL_SRC_FILES :=	\
		sensors.cpp 			\
		SensorBase.cpp			\
		LightSensor.cpp			\
		ProximitySensor.cpp		\
		Lis3dh.cpp				\
		Mpu3050.cpp				\
		Bmp180.cpp				\
		InputEventReader.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

commonCflags   := $(remote_api_defines)
commonCflags   += $(remote_api_enables)

LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += hardware/libhardware/include
LOCAL_C_INCLUDES += hardware/libhardware/include/hardware

LOCAL_MODULE:=sensors.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SHARED_LIBRARIES += \
    libc \
    libutils \
    libcutils \
    liblog \
    libhardware \
    libdl

LOCAL_SRC_FILES := sensors_os_hal.c

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif #BUILD_TINY_ANDROID
