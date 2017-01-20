LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    Mic.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(ANDROID4)/frameworks/base/include \

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libnativehelper \
    libfsccore \
    libgui \
    libandroid_runtime


LOCAL_MODULE := libmic_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

