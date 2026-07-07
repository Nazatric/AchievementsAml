LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE := achievementsaml
LOCAL_SRC_FILES := main.cpp logger.cpp
LOCAL_CFLAGS += -O2 -DNDEBUG -std=c++17
LOCAL_C_INCLUDES += .
LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
