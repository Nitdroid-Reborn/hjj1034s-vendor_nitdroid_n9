/*
 * Copyright (C) 2012-2013 Alexey Roslyakov <alexey.roslyakov@newsycat.com>, The NITDroid Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/ioctl.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <utils/Trace.h>
#include <hardware/hwcomposer.h>

#include <EGL/egl.h>

/*****************************************************************************/

#define HWC_VSYNC_THREAD_NAME "hwcVsyncThread"

static const char LCD_BLANK[] = "/sys/class/graphics/fb0/blank";
static const char DISABLE_TS[] = "/sys/devices/platform/i2c_omap.2/i2c-2/2-004b/disable_ts";
static const char CPR_COEF[] = "/sys/devices/platform/omapdss/manager0/cpr_coef";
static const char CPR_ENABLE[] = "/sys/devices/platform/omapdss/manager0/cpr_enable";

struct vsync_state {
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    bool enable;
    bool fakevsync;
};

struct hwc_context_t {
    hwc_composer_device_1 device;
    /* our private state goes below here */
    const hwc_procs_t* proc;
    int fb;
    struct vsync_state vstate;
};


#define _IOC(dir,type,nr,size)                  \
    (((dir)  << _IOC_DIRSHIFT) |                \
     ((type) << _IOC_TYPESHIFT) |               \
     ((nr)   << _IOC_NRSHIFT) |                 \
     ((size) << _IOC_SIZESHIFT))
#define _IOW(type,nr,size)      _IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))

#define OMAP_IOW(num, dtype)	_IOW('O', num, dtype)
#define OMAP_IO(num)            _IO('O', num)

static int fbUpdateWindow(int fd, int x=0, int y=0, int width=854, int height=480)
{
    struct omapfb_update_window {
        __u32 x, y;
        __u32 width, height;
        __u32 format;
        __u32 out_x, out_y;
        __u32 out_width, out_height;
        __u32 reserved[8];
    };

    const unsigned int OMAPFB_SYNC_GFX = OMAP_IO(37);
    const unsigned int OMAPFB_WAITFORVSYNC = OMAP_IO(57);
    const unsigned int OMAPFB_UPDATE_WINDOW = OMAP_IOW(54, struct omapfb_update_window);
    const unsigned int OMAPFB_SET_UPDATE_MODE = OMAP_IOW(40, int);

    struct omapfb_update_window update;
    memset(&update, 0, sizeof(omapfb_update_window));
    update.format = 0; /*OMAPFB_COLOR_RGB565*/
    update.x = x;
    update.y = y;
    update.out_x = x;
    update.out_y = y;
    update.out_width = width;
    update.out_height = height;
    update.width = width;
    update.height = height;
    //ioctl(fd, OMAPFB_WAITFORVSYNC, 0);
    if (ioctl(fd, OMAPFB_UPDATE_WINDOW, &update) < 0) {
        ALOGE("Could not ioctl(OMAPFB_UPDATE_WINDOW): %s", strerror(errno));
    }
    //ioctl(fd, OMAPFB_SYNC_GFX, 0);

    //ALOGD("fbUpdateWindow(%d, %d, %d, %d)", x, y, width, height);
    return 0;
}

static int writeStringToFile(const char *file, const char *data)
{
    int res = 0;
    int f = open(file, O_WRONLY);
    if (f <= 0) {
        ALOGE("writeStringToFile can't open file %s: %s", file, strerror(errno));
        return errno;
    }

    int len = strlen(data);
    if (len != write(f, data, len)) {
        ALOGE("writeStringToFile(%s, %s) failed: %s", file, data, strerror(errno));
        res = errno;
    }

    close(f);
    return res;
}

static void *vsync_loop(void *param)
{
    ALOGD("vsync_loop");
    hwc_context_t * ctx = reinterpret_cast<hwc_context_t *>(param);

    char thread_name[64] = HWC_VSYNC_THREAD_NAME;
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY +
                android::PRIORITY_MORE_FAVORABLE);
    int dpy = HWC_DISPLAY_PRIMARY;

    do {
        pthread_mutex_lock(&ctx->vstate.lock);
        while (ctx->vstate.enable == false) {
            pthread_cond_wait(&ctx->vstate.cond, &ctx->vstate.lock);
        }
        pthread_mutex_unlock(&ctx->vstate.lock);

        usleep(16000);
        uint64_t cur_timestamp = systemTime();
        ctx->proc->vsync(ctx->proc, dpy, cur_timestamp);
        //ALOGD("vsync: %lu", cur_timestamp);
    } while(true);

    return 0;
}

static void init_vsync_thread(hwc_context_t* ctx)
{
    int ret;
    pthread_t vsync_thread;
    ALOGD("Initializing VSYNC Thread");
    ret = pthread_create(&vsync_thread, NULL, vsync_loop, (void*) ctx);
    if (ret) {
        ALOGE("%s: failed to create %s: %s", __FUNCTION__,
              HWC_VSYNC_THREAD_NAME, strerror(ret));
    }
}

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 2,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "NITDroid hwcomposer module",
        author: "Alexey Roslyakov, The NITDroid project",
        methods: &hwc_module_methods,
        dso: 0,
        reserved: {0},
    }
};

/*****************************************************************************/

static void dump_layer(hwc_layer_1 const* l) {
    ALOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform, l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);
}

static int hwc_prepare(hwc_composer_device_1 *dev,
                       size_t numDisplays, hwc_display_contents_1_t** displays)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    {
        const unsigned int OMAPFB_SYNC_GFX = OMAP_IO(37);
        ioctl(ctx->fb, OMAPFB_SYNC_GFX, 0);
    }
    //fbUpdateWindow(ctx->fb);
    return 0;
}

static int hwc_set(hwc_composer_device_1 *dev,
                   size_t numDisplays, hwc_display_contents_1_t** displays)
{
    int ret = 0;
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    for (uint32_t i = 0; i <numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        //XXX: Actually handle the multiple displays
        if (list) {

            EGLBoolean success = eglSwapBuffers((EGLDisplay)list->dpy,
                                                (EGLSurface)list->sur);
            fbUpdateWindow(ctx->fb);
        }
    }

    return ret;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, int disp,
                            int event, int enable)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    int res = 0;

    pthread_mutex_lock(&ctx->vstate.lock);
    switch(event) {
        case HWC_EVENT_VSYNC:
            if (ctx->vstate.enable == enable)
                break;
            ctx->vstate.enable = !!enable;
            pthread_cond_signal(&ctx->vstate.cond);
            //ALOGD("VSYNC state changed to %s", (enable)?"ENABLED":"DISABLED");
            break;
      default:
          res = -EINVAL;
    }
    pthread_mutex_unlock(&ctx->vstate.lock);

    return res;
}

static int hwc_blank(struct hwc_composer_device_1* dev, int dpy, int blank)
{
    ALOGD("hwc_blank: %d", blank);
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    writeStringToFile(LCD_BLANK, blank ? "1\n" : "0\n");
    writeStringToFile(DISABLE_TS, blank ? "1\n" : "0\n");

    if (!blank) {
        // set user-defined (see /default.prop) colour profile for LCD
        char propValue[PROPERTY_VALUE_MAX];
        if ( property_get("hw.lcd.colourprofile", propValue, NULL)) {
            ALOGD("Colour profile: %s", propValue);
            writeStringToFile(CPR_COEF, propValue);
            writeStringToFile(CPR_ENABLE, "1");
        }
        usleep(10000);
        for(int i = 0; i < 10; i++)
            fbUpdateWindow(ctx->fb);
    }

    return 0;
}

static int hwc_query(struct hwc_composer_device_1* dev,
                     int param, int* value)
{
    int supported = HWC_DISPLAY_PRIMARY_BIT;

    switch (param) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // Not supported for now
        value[0] = 0;
        break;
    case HWC_VSYNC_PERIOD: //Not used for hwc > 1.1
        value[0] = 0;
        ALOGI("fps: %d", value[0]);
        break;
    case HWC_DISPLAY_TYPES_SUPPORTED:
        value[0] = supported;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
                              hwc_procs_t const* procs)
{
    ALOGD("hwc_registerProcs");
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    ctx->proc = procs;
    init_vsync_thread(ctx);
}

static void hwc_dump(struct hwc_composer_device_1* dev, char *buff, int buff_len)
{
    ALOGD("hwc_dump");
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (ctx) {
        free(ctx);
    }
    return 0;
}

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;
    if (!strcmp(name, HWC_HARDWARE_COMPOSER)) {
        struct hwc_context_t *dev;
        dev = (hwc_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));
        pthread_mutex_init(&(dev->vstate.lock), NULL);
        pthread_cond_init(&(dev->vstate.cond), NULL);

        /* initialize the procs */
        dev->device.common.tag          = HARDWARE_DEVICE_TAG;
        dev->device.common.version      = HWC_DEVICE_API_VERSION_1_0;
        dev->device.common.module       = const_cast<hw_module_t*>(module);
        dev->device.common.close        = hwc_device_close;
        dev->device.prepare             = hwc_prepare;
        dev->device.set                 = hwc_set;
        dev->device.eventControl        = hwc_eventControl;
        dev->device.blank               = hwc_blank;
        dev->device.query               = hwc_query;
        dev->device.registerProcs       = hwc_registerProcs;
        dev->device.dump                = hwc_dump;
        *device = &dev->device.common;
        status = 0;

        dev->fb = open("/dev/graphics/fb0", O_RDWR, 0);
        if (dev->fb < 0) {
            status = errno;
            ALOGE("Can't open framebuffer device: %s", strerror(errno));
        }
    }
    return status;
}
