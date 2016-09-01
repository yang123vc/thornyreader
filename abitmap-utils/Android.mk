LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := abitmap-utils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	abitmap-utils.cpp

LOCAL_ARM_MODE := $(APP_ARM_MODE)

include $(BUILD_STATIC_LIBRARY)
