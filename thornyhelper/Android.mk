LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := thornyhelper

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	bitmaputils.cpp

LOCAL_ARM_MODE := $(APP_ARM_MODE)

include $(BUILD_STATIC_LIBRARY)
