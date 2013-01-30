LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := crashpin
LOCAL_SRC_FILES := main.cpp ../../src/crashpin.cpp ../../src/map.cpp# ../../src/stacktrace.cpp
LOCAL_LDLIBS    := -llog
LOCAL_CPPFLAGS  := -g
include $(BUILD_SHARED_LIBRARY)