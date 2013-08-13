# This is a generic product that isn't specialized for a specific device.
# It includes the base Android platform. If you need Google-specific features,
# you should derive from generic_with_google.mk

#$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# This is the list of apps included in the generic AOSP build


PRODUCT_PACKAGES := \
    AccountAndSyncSettings \
    DeskClock \
    AlarmProvider \
    Bluetooth \
    Calculator \
    Calendar \
    Camera \
    CellBroadcastReceiver \
    CertInstaller \
    DrmProvider \
    Email \
    Gallery2 \
    LatinIME \
    Launcher2 \
    Mms \
    Music \
    Phone \
    Provision \
    Protips \
    QuickSearchBox \
    Settings \
    Sync \
    SystemUI \
    Updater \
    CalendarProvider \
    SyncProvider \
    IM \
    VoiceDialer \
    VideoEditor \
    Development \
    SpareParts \
    SoundRecorder

# Live Wallpapers
PRODUCT_PACKAGES += \
    LiveWallpapers \
    LiveWallpapersPicker \
    VisualizationWallpapers \
    librs_jni
##

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/busybox/busybox:root/bin/busybox \
    $(LOCAL_PATH)/init.rc:root/init.rc \
    $(LOCAL_PATH)/init.nokiarm-696board.rc:root/init.nokiarm-696board.rc \
    $(LOCAL_PATH)/init.nokiarm-680board.rc:root/init.nokiarm-680board.rc \
    $(LOCAL_PATH)/init.nokiarm-696board.usb.rc:root/init.nokiarm-696board.usb.rc \
    $(LOCAL_PATH)/ueventd.nokiarm-696board.rc:root/ueventd.nokiarm-696board.rc \
    $(LOCAL_PATH)/ueventd.nokiarm-680board.rc:root/ueventd.nokiarm-680board.rc \
    $(LOCAL_PATH)/etc/media_profiles.xml:system/etc/media_profiles.xml \
    $(LOCAL_PATH)/fstab.nokiarm-696board:root/fstab.nokiarm-696board \
    $(LOCAL_PATH)/etc/modem.conf:system/etc/modem.conf \
    $(LOCAL_PATH)/etc/gps.conf:system/etc/gps.conf \
    $(LOCAL_PATH)/etc/harmount.sh:system/etc/harmount.sh \
    $(LOCAL_PATH)/etc/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/etc/excluded-input-devices.xml:system/etc/excluded-input-devices.xml \
    $(LOCAL_PATH)/system/xbin/rr:system/xbin/rr
##

# Input device calibration files
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/Atmel_mXT_Touchscreen.idc:system/usr/idc/Atmel_mXT_Touchscreen.idc \
    $(LOCAL_PATH)/TWL4030_Keypad.idc:system/usr/idc/TWL4030_Keypad.idc \
    $(LOCAL_PATH)/TWL4030_Keypad.kl:system/usr/keylayout/TWL4030_Keypad.kl \
    $(LOCAL_PATH)/TWL4030_Keypad.kcm:system/usr/keychars/TWL4030_Keypad.kcm

# Permissions

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.telephony.cdma.xml:system/etc/permissions/android.hardware.telephony.cdma.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
##

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

DEVICE_PACKAGE_OVERLAYS := vendor/nitdroid/n9/overlay

# Overrides
PRODUCT_BRAND := nokia
PRODUCT_DEVICE := n9
PRODUCT_NAME := n9
PRODUCT_MODEL := Nokia N9

# This is a high DPI device, so add the hdpi pseudo-locale
PRODUCT_LOCALES += en_US hdpi
