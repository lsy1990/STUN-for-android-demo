#copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This makefile supplies the rules for building a library of JNI code for
# use by our example platform shared library.

LOCAL_PATH:= $(call my-dir)
LOCAL_ICS_PATH:=$(LOCAL_PATH)/../../..
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# This is the target being built.
LOCAL_MODULE:= libnetworkutils

# All of the source files that we will compile.
LOCAL_SRC_FILES:= adapters.cpp \
                  polling.cpp \
                  recvfromex.cpp  \
                  resolvehostname.cpp \
                  stunsocket.cpp  \
                  socket/ifaddrs.cpp \
#TODO: specify source files

# All of the shared libraries we link against.
LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils  \
    libhardware \
    libhardware_legacy \
    libnativehelper \
    libcommon \
    libstuncore \
    libnetutils \
# No static libraries.
LOCAL_STATIC_LIBRARIES :=  libstdc++  \
                              libgnustl_static \
# Also need the  headers.
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../stuncore/
LOCAL_C_INCLUDES += $(LOCAL_ICS_PATH)/prebuilts/ndk/8/sources/cxx-stl/gnu-libstdc++/include/
LOCAL_C_INCLUDES += $(LOCAL_ICS_PATH)/prebuilts/ndk/8/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/include/


# No specia compiler flags.
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wuninitialized  -Wuninitialized -Wmissing-field-initializers

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# This is the target being built.
LOCAL_MODULE:= libnetworkutils

# All of the source files that we will compile.
LOCAL_SRC_FILES:= adapters.cpp \
                  polling.cpp \
                  recvfromex.cpp  \
                  resolvehostname.cpp \
                  stunsocket.cpp  \
                  socket/ifaddrs.cpp
#TODO: specify source files

# All of the shared libraries we link against.
LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils  \
    libnetutils \
    libhardware \
    libhardware_legacy \
    libnativehelper \
    libcommon \
    libstuncore \

# No static libraries.
LOCAL_STATIC_LIBRARIES :=  libstdc++  \
                           libgnustl_static \
# Also need the  headers.
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../stuncore/
LOCAL_C_INCLUDES += $(LOCAL_ICS_PATH)/prebuilts/ndk/8/sources/cxx-stl/gnu-libstdc++/include/
LOCAL_C_INCLUDES += $(LOCAL_ICS_PATH)/prebuilts/ndk/8/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/include/


# No specia compiler flags.
LOCAL_CFLAGS += -Wno-non-virtual-dtor -Wuninitialized  -Wuninitialized -Wmissing-field-initializers

include $(BUILD_SHARED_LIBRARY)
