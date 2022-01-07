#ifdef X11

#include "common.h"
#include "x11-utils.h"
#ifdef IMLIB2
#include <Imlib2.h>
#endif /* IMLIB2 */
#include <unistd.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>


static void reset_root_atoms(Display *display, Window root, Pixmap pixmap);

static Display *display = NULL;
static int x11_initialized = 0;

void reset_root_atoms(Display *display, Window root, Pixmap pixmap) {
    Atom atom_root, atom_eroot, type;
    unsigned char *data_root, *data_eroot;
    int format;
    unsigned long length, after;

    atom_root = XInternAtom(display, "_XROOTMAP_ID", True);
    atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", True);

    // doing this to clean up after old background
    if (atom_root != None && atom_eroot != None) {
        XGetWindowProperty(display, root, atom_root, 0L, 1L, False,
                AnyPropertyType, &type, &format, &length, &after,
                &data_root);

        if (type == XA_PIXMAP) {
            XGetWindowProperty(display, root, atom_eroot, 0L, 1L, False,
                    AnyPropertyType, &type, &format, &length, &after,
                    &data_eroot);

            if (data_root && data_eroot && type == XA_PIXMAP &&
                    *((Pixmap *)data_root) == *((Pixmap *)data_eroot))
                XKillClient(display, *((Pixmap *)data_root));
        }
    }

    atom_root = XInternAtom(display, "_XROOTPMAP_ID", False);
    atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

    // setting new background atoms
    XChangeProperty(display, root, atom_root, XA_PIXMAP, 32,
            PropModeReplace, (unsigned char *)&pixmap, 1);
    XChangeProperty(display, root, atom_eroot, XA_PIXMAP, 32,
            PropModeReplace, (unsigned char *)&pixmap, 1);
}

int x11_init(void) {
    if (x11_initialized)
        return 1;

    /* open x11 display */
    display = XOpenDisplay(NULL);
    if (!display) {
        report(R_ERROR, "Unable to open X11 display");
        return 0;
    }

    XRRSelectInput(display, XDefaultRootWindow(display),
            RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask |
            RROutputChangeNotifyMask | RROutputPropertyNotifyMask);

    x11_initialized = 1;
    return 1;
}

int x11_screen_has_changed(void) {
    int have_rr, rr_event_base, rr_error_base;
    int screen_changed = 0;
    if (!display)
        return 0;
    XEvent event;
    have_rr = XRRQueryExtension (display, &rr_event_base, &rr_error_base);
    if (!have_rr)
        return 0;
    while (XCheckTypedEvent(display, rr_event_base + RRScreenChangeNotify, &event)
            || XCheckTypedEvent(display, rr_event_base + RRNotify, &event)) {
        /* if (event.type == rr_event_base + RRScreenChangeNotify) */
        /*     fprintf(stderr, "RRScreenChangeNotify\n"); */
        /* else if (event.type == rr_event_base + RRNotify) */
        /*     fprintf(stderr, "RRNotify\n"); */
        /* else */
        /*     fprintf(stderr, "Unknown: %d\n", event.type); */
        screen_changed = 1;
    }
    return screen_changed;
}

int x11_wallpaper_all(const char *path) {
#ifdef IMLIB2
    int dpy_width, dpy_height, new_width, new_height;
    int depth, color_map, screen, i, status;
    double screen_ratio, image_ratio;
    Visual *vis;
    Window root;
    Pixmap pixmap;
    Imlib_Image image, cropped_image;
    XRRScreenResources *screen_res;
    XRRCrtcInfo *crtc_info;

    if (!display)
        return 0;

    image = imlib_load_image(path);
    if (!image)
        return 0;

    screen = DefaultScreen(display);

    dpy_width = DisplayWidth(display, screen);
    dpy_height = DisplayHeight(display, screen);
    depth = DefaultDepth(display, screen);
    color_map = DefaultColormap(display, screen);
    vis = DefaultVisual(display, screen);

    root = RootWindow(display, screen);
    pixmap = XCreatePixmap(display, root, dpy_width, dpy_height, depth);

    imlib_context_set_display(display);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(color_map);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color_range(imlib_create_color_range());
    imlib_context_set_image(image);

    screen_res = XRRGetScreenResources(display, DefaultRootWindow(display));
    status = 1;
    for (i = 0; i < screen_res->noutput; i++) {
        crtc_info = XRRGetCrtcInfo(display, screen_res, screen_res->crtcs[i]);
        if (crtc_info->width > 0 && crtc_info->height > 0) {
            screen_ratio = ((double) crtc_info->width) / ((double) crtc_info->height);
            image_ratio = ((double) imlib_image_get_width()) / ((double) imlib_image_get_height());
            if (image_ratio < screen_ratio) {
                new_height = (int) ((((double) imlib_image_get_width()) * ((double) crtc_info->height)) / ((double) crtc_info->width));
                cropped_image = imlib_create_cropped_scaled_image(0, (imlib_image_get_height() - new_height) / 2,
                        imlib_image_get_width(), new_height,
                        crtc_info->width, crtc_info->height);
            } else if (image_ratio > screen_ratio) {
                new_width = (int) ((((double) imlib_image_get_height()) * ((double) crtc_info->width)) / ((double) crtc_info->height));
                cropped_image = imlib_create_cropped_scaled_image((imlib_image_get_width() - new_width) / 2, 0,
                        new_width, imlib_image_get_height(),
                        crtc_info->width, crtc_info->height);
            } else {
                cropped_image = imlib_create_cropped_scaled_image(0, 0,
                        imlib_image_get_width(), imlib_image_get_height(),
                        crtc_info->width, crtc_info->height);
            }

            if (!cropped_image)
                return 0;

            imlib_context_set_image(cropped_image);
            imlib_render_image_on_drawable(crtc_info->x, crtc_info->y);
            imlib_free_image();
            imlib_context_set_image(image);
            XRRFreeCrtcInfo(crtc_info);
        }
    }
    XRRFreeScreenResources(screen_res);

    reset_root_atoms(display, root, pixmap);
    XKillClient(display, AllTemporary);
    XSetCloseDownMode(display, RetainTemporary);
    XSetWindowBackgroundPixmap(display, root, pixmap);
    XClearWindow(display, root);
    XFlush(display);
    XSync(display, False);

    imlib_free_color_range();
    imlib_free_image();

    return status;
#else /* IMLIB2 */
    return 0;
#endif /* IMLIB2 */
}

void x11_deinit(void) {
    if (!x11_initialized)
        return;
    XCloseDisplay(display);
}

#endif /* X11 */
