#ifndef __STEREO_HELPER_H__
#define __STEREO_HELPER_H__

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "nvstusb.h"

// makes sure that the nvstusb refresh rate is in sync with what X11 thinks it
// is
void stereo_helper_config_refresh_rate(struct nvstusb_context *ctx);


void stereo_helper_config_refresh_rate(struct nvstusb_context *ctx) {
    Display *display = XOpenDisplay(0);
    double display_num = DefaultScreen(display);
    XF86VidModeModeLine mode_line;
    int pixel_clk = 0;
    XF86VidModeGetModeLine(display, display_num, &pixel_clk, &mode_line);
    double frame_rate = (double) pixel_clk * 1000.0 / mode_line.htotal / mode_line.vtotal;
    printf("Detected refresh rate of %f Hz.\n", frame_rate);
    nvstusb_set_rate(ctx, frame_rate);
}

#endif // __STEREO_HELPER_H__
