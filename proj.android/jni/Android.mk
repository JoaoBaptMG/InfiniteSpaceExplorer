LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos/audio/include)
$(call import-add-path,$(LOCAL_PATH)/../../gpg-cpp-sdk/android)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

CLASS_FILES := $(wildcard $(LOCAL_PATH)/../../Classes/*.cpp)
CLASS_FILES := $(CLASS_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES := Java_joaobapt_CommonAlertListener.cpp
LOCAL_SRC_FILES += Java_joaobapt_PictureDownloader.cpp
LOCAL_SRC_FILES += Java_joaobapt_FacebookManager.cpp
LOCAL_SRC_FILES += hellocpp/main.cpp
LOCAL_SRC_FILES += $(CLASS_FILES)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Classes

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END


LOCAL_STATIC_LIBRARIES := cocos2dx_static
LOCAL_STATIC_LIBRARIES += libgpg-1

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,../../gpg-cpp-sdk/android)
$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END
