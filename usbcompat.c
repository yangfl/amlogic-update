#ifndef WITH_LIBUSB1
/* pass */
#else
#include <errno.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libusb.h>
#pragma GCC diagnostic pop

libusb_context *ctx = NULL;
int usb_debug = 0;
struct usb_bus *usb_busses = NULL;

#ifdef LIBUSB_1_0_SONAME
static void __attribute__ ((constructor)) _usb_init (void)
{
	libusb_dl_init ();
}
#endif

static void __attribute__ ((destructor)) _usb_exit (void)
{
	if (ctx) {
		libusb_exit (ctx);
		ctx = NULL;
	}
#ifdef LIBUSB_1_0_SONAME
	libusb_dl_exit ();
#endif
}

#endif
