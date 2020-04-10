//src: https://gitlab.freedesktop.org/xorg/xserver/issues/597
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

static Window create_win(Display *dpy)
{
    XIEventMask mask;

    Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 200,
            200, 0, 0, WhitePixel(dpy, 0));
    Window subwindow = XCreateSimpleWindow(dpy, win, 50, 50, 50, 50, 0, 0,
            BlackPixel(dpy, 0));

    XSelectInput(dpy, win, StructureNotifyMask);

    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_RawMotion);
    mask.mask = calloc(mask.mask_len, sizeof(unsigned char));
    //memset(mask.mask, 0, mask.mask_len);
    explicit_bzero(mask.mask, mask.mask_len);

    XISetMask(mask.mask, XI_RawMotion);

    XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

    free(mask.mask);
    XMapWindow(dpy, subwindow);
    XMapWindow(dpy, win);
    XSync(dpy, True);
    return win;
}

static void print_rawmotion(XIRawEvent *event)
{
    int i;
    double *raw_valuator = event->raw_values,
           *valuator = event->valuators.values;

    printf("    device: %d (%d)\n", event->deviceid, event->sourceid);

    for (i = 0; i < event->valuators.mask_len * 8; i++)
    {
        if (XIMaskIsSet(event->valuators.mask, i))
        {
            printf("  acceleration on valuator %d: %f <%f %f>\n",
                    i, *valuator - *raw_valuator, *valuator, *raw_valuator);
            valuator++;
            raw_valuator++;
        }
    }
}

int main (int argc, char **argv)
{
    Display *dpy;
    int xi_opcode, event, error;
    int major = 2, minor = 1;
    Window win;
    XEvent ev;

    dpy = XOpenDisplay(NULL);

    if (!dpy) {
        fprintf(stderr, "Failed to open display.\n");
        return -1;
    }

    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
        printf("X Input extension not available.\n");
        return -1;
    }

    if (XIQueryVersion(dpy, &major, &minor) != Success) {
        return -1;
    }

    win = create_win(dpy);

    while(1)
    {
        XGenericEventCookie *cookie = &ev.xcookie;

        XNextEvent(dpy, &ev);

        if (ev.type == MapNotify) {
            XGrabPointer(dpy,
                win
                //DefaultRootWindow(dpy)
                , True, 0, GrabModeAsync,
                    GrabModeAsync,
                    win
                    //DefaultRootWindow(dpy)
                    , None, CurrentTime);
        }

        if (cookie->type != GenericEvent ||
            cookie->extension != xi_opcode ||
            !XGetEventData(dpy, cookie))
            continue;

        printf("EVENT TYPE %d\n", cookie->evtype);
        if (cookie->evtype == XI_RawMotion)
            print_rawmotion(cookie->data);

        XFreeEventData(dpy, cookie);
    }

    XCloseDisplay(dpy);
    return 0;
}

