LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= motor_jni
LOCAL_SRC_FILES:= motor_jni.c
#LOCAL_LDLIBS := -llog
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

include $(BUILD_SHARED_LIBRARY)