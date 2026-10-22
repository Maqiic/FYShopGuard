# Build an injectable Mach-O dylib (not a jailbreak .deb).
TARGET := iphone:clang:latest:14.0
ARCHS := arm64
FINALPACKAGE := 1
include $(THEOS)/makefiles/common.mk
LIBRARY_NAME := FYShopGuard
FYShopGuard_FILES := Tweak.c
FYShopGuard_LIBRARIES := objc
FYShopGuard_FRAMEWORKS := Foundation
FYShopGuard_CFLAGS := -std=c11 -Wall -Wextra -Wno-cast-function-type
FYShopGuard_INSTALL_PATH := /usr/lib
include $(THEOS)/makefiles/library.mk
