#ifndef AML_USB_COMPAT_H
#define AML_USB_COMPAT_H
#ifndef WITH_LIBUSB1
#include <usb.h>
#else
#include <errno.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libusb.h>
#pragma GCC diagnostic pop

/*
 * USB spec information
 *
 * This is all stuff grabbed from various USB specs and is pretty much
 * not subject to change
 */

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE		0	/* for DeviceClass */
#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PRINTER		7
#define USB_CLASS_PTP			6
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			9
#define USB_CLASS_DATA			10
#define USB_CLASS_VENDOR_SPEC		0xff

/*
 * Descriptor types
 */
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_DT_HID			0x21
#define USB_DT_REPORT			0x22
#define USB_DT_PHYSICAL			0x23
#define USB_DT_HUB			0x29

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE		18
#define USB_DT_CONFIG_SIZE		9
#define USB_DT_INTERFACE_SIZE		9
#define USB_DT_ENDPOINT_SIZE		7
#define USB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE		7

/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
};

/* String descriptor */
struct usb_string_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t wData[1];
};

/* HID descriptor */
struct usb_hid_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdHID;
	uint8_t  bCountryCode;
	uint8_t  bNumDescriptors;
	/* uint8_t  bReportDescriptorType; */
	/* uint16_t wDescriptorLength; */
	/* ... */
};

/* Endpoint descriptor */
#define USB_MAXENDPOINTS	32
struct usb_endpoint_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bEndpointAddress;
	uint8_t  bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t  bInterval;
	uint8_t  bRefresh;
	uint8_t  bSynchAddress;

	unsigned char *extra;	/* Extra descriptors */
	int extralen;
};

#define USB_ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_TYPE_MASK		0x03    /* in bmAttributes */
#define USB_ENDPOINT_TYPE_CONTROL	0
#define USB_ENDPOINT_TYPE_ISOCHRONOUS	1
#define USB_ENDPOINT_TYPE_BULK		2
#define USB_ENDPOINT_TYPE_INTERRUPT	3

/* Interface descriptor */
#define USB_MAXINTERFACES	32
struct usb_interface_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bInterfaceNumber;
	uint8_t  bAlternateSetting;
	uint8_t  bNumEndpoints;
	uint8_t  bInterfaceClass;
	uint8_t  bInterfaceSubClass;
	uint8_t  bInterfaceProtocol;
	uint8_t  iInterface;

	struct usb_endpoint_descriptor *endpoint;

	unsigned char *extra;	/* Extra descriptors */
	int extralen;
};

#define USB_MAXALTSETTING	128	/* Hard limit */
struct usb_interface {
	struct usb_interface_descriptor *altsetting;

	int num_altsetting;
};

/* Configuration descriptor information.. */
#define USB_MAXCONFIG		8
struct usb_config_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t wTotalLength;
	uint8_t  bNumInterfaces;
	uint8_t  bConfigurationValue;
	uint8_t  iConfiguration;
	uint8_t  bmAttributes;
	uint8_t  MaxPower;

	struct usb_interface *interface;

	unsigned char *extra;	/* Extra descriptors */
	int extralen;
};

/* Device descriptor */
struct usb_device_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
};

struct usb_ctrl_setup {
	uint8_t  bRequestType;
	uint8_t  bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
};

/*
 * Standard requests
 */
#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
/* 0x02 is reserved */
#define USB_REQ_SET_FEATURE		0x03
/* 0x04 is reserved */
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C

#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

/*
 * Various libusb API related stuff
 */

#define USB_ENDPOINT_IN			0x80
#define USB_ENDPOINT_OUT		0x00

/* Error codes */
#define USB_ERROR_BEGIN			500000

/* Data types */
struct usb_device;
struct usb_bus;

/*
 * To maintain compatibility with applications already built with libusb,
 * we must only add entries to the end of this structure. NEVER delete or
 * move members and only change types if you really know what you're doing.
 */
struct usb_device {
  struct usb_device *next, *prev;

  char filename[PATH_MAX + 1];

  struct usb_bus *bus;

  struct usb_device_descriptor descriptor;
  struct usb_config_descriptor *config;

  void *dev;		/* Darwin support */

  uint8_t devnum;

  unsigned char num_children;
  struct usb_device **children;
};

struct usb_bus {
  struct usb_bus *next, *prev;

  char dirname[PATH_MAX + 1];

  struct usb_device *devices;
  uint32_t location;

  struct usb_device *root_dev;
};

struct usb_dev_handle;
typedef struct usb_dev_handle usb_dev_handle;

/* Some quick and generic macros for the simple kind of lists we use */
#define LIST_ADD(begin, ent) \
	do { \
	  if (begin) { \
	    ent->next = begin; \
	    ent->next->prev = ent; \
	  } else \
	    ent->next = NULL; \
	  ent->prev = NULL; \
	  begin = ent; \
	} while(0)

#define LIST_DEL(begin, ent) \
	do { \
	  if (ent->prev) \
	    ent->prev->next = ent->next; \
	  else \
	    begin = ent->next; \
	  if (ent->next) \
	    ent->next->prev = ent->prev; \
	  ent->prev = NULL; \
	  ent->next = NULL; \
	} while (0)

struct usb_dev_handle {
	libusb_device_handle *handle;
	struct usb_device *device;

	/* libusb-0.1 is buggy w.r.t. interface claiming. it allows you to claim
	 * multiple interfaces but only tracks the most recently claimed one,
	 * which is used for usb_set_altinterface(). we clone the buggy behaviour
	 * here. */
	int last_claimed_interface;
};

#define _usbi_log(level, fmt, ...)
#define usbi_dbg(fmt, ...)
#define usbi_info(fmt, ...) _usbi_log(LOG_LEVEL_INFO, fmt, ## __VA_ARGS__)
#define usbi_warn(fmt, ...) _usbi_log(LOG_LEVEL_WARNING, fmt, ## __VA_ARGS__)
#define usbi_err(fmt, ...) _usbi_log(LOG_LEVEL_ERROR, fmt, ## __VA_ARGS__)

#define compat_err(e) -(errno=libusb_to_errno(e))

extern libusb_context *ctx;
extern int usb_debug;
extern struct usb_bus *usb_busses;

static inline int libusb_to_errno(int result)
{
	switch (result) {
	case LIBUSB_SUCCESS:
		return 0;
	case LIBUSB_ERROR_IO:
		return EIO;
	case LIBUSB_ERROR_INVALID_PARAM:
		return EINVAL;
	case LIBUSB_ERROR_ACCESS:
		return EACCES;
	case LIBUSB_ERROR_NO_DEVICE:
		return ENXIO;
	case LIBUSB_ERROR_NOT_FOUND:
		return ENOENT;
	case LIBUSB_ERROR_BUSY:
		return EBUSY;
	case LIBUSB_ERROR_TIMEOUT:
		return ETIMEDOUT;
	case LIBUSB_ERROR_OVERFLOW:
		return EOVERFLOW;
	case LIBUSB_ERROR_PIPE:
		return EPIPE;
	case LIBUSB_ERROR_INTERRUPTED:
		return EINTR;
	case LIBUSB_ERROR_NO_MEM:
		return ENOMEM;
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return ENOSYS;
	default:
		return ERANGE;
	}
}

inline void usb_init(void)
{
	int r;
	usbi_dbg("");

	if (!ctx) {
		r = libusb_init(&ctx);
		if (r < 0) {
			usbi_err("initialization failed!");
			return;
		}

		/* usb_set_debug can be called before usb_init */
		if (usb_debug)
			libusb_set_debug(ctx, 3);
	}
}

inline char *usb_strerror(void)
{
	return strerror(errno);
}

static inline int find_busses(struct usb_bus **ret)
{
	libusb_device **dev_list = NULL;
	struct usb_bus *busses = NULL;
	struct usb_bus *bus;
	int dev_list_len = 0;
	int i;
	int r;

	r = libusb_get_device_list(ctx, &dev_list);
	if (r < 0) {
		usbi_err("get_device_list failed with error %d", r);
		return compat_err(r);
	}

	if (r == 0) {
		libusb_free_device_list(dev_list, 1);
		/* no buses */
		return 0;
	}

	/* iterate over the device list, identifying the individual busses.
	 * we use the location field of the usb_bus structure to store the
	 * bus number. */

	dev_list_len = r;
	for (i = 0; i < dev_list_len; i++) {
		libusb_device *dev = dev_list[i];
		uint8_t bus_num = libusb_get_bus_number(dev);

		/* if we already know about it, continue */
		if (busses) {
			bus = busses;
			int found = 0;
			do {
				if (bus_num == bus->location) {
					found = 1;
					break;
				}
			} while ((bus = bus->next) != NULL);
			if (found)
				continue;
		}

		/* add it to the list of busses */
		bus = (struct usb_bus *) malloc(sizeof(*bus));
		if (!bus)
			goto err;

		memset(bus, 0, sizeof(*bus));
		bus->location = bus_num;
		sprintf(bus->dirname, "%03d", bus_num);
		LIST_ADD(busses, bus);
	}

	libusb_free_device_list(dev_list, 1);
	*ret = busses;
	return 0;

err:
	bus = busses;
	while (bus) {
		struct usb_bus *tbus = bus->next;
		free(bus);
		bus = tbus;
	}
	return -ENOMEM;
}

inline int usb_find_busses(void)
{
	struct usb_bus *new_busses = NULL;
	struct usb_bus *bus;
	int changes = 0;
	int r;

	/* libusb-1.0 initialization might have failed, but we can't indicate
	 * this with libusb-0.1, so trap that situation here */
	if (!ctx)
		return 0;

	usbi_dbg("");
	r = find_busses(&new_busses);
	if (r < 0) {
		usbi_err("find_busses failed with error %d", r);
		return r;
	}

	/* walk through all busses we already know about, removing duplicates
	 * from the new list. if we do not find it in the new list, the bus
	 * has been removed. */

	bus = usb_busses;
	while (bus) {
		struct usb_bus *tbus = bus->next;
		struct usb_bus *nbus = new_busses;
		int found = 0;
		usbi_dbg("in loop");

		while (nbus) {
			struct usb_bus *tnbus = nbus->next;

			if (bus->location == nbus->location) {
				LIST_DEL(new_busses, nbus);
				free(nbus);
				found = 1;
				break;
			}
			nbus = tnbus;
		}

		if (!found) {
			/* bus removed */
			usbi_dbg("bus %d removed", bus->location);
			changes++;
			LIST_DEL(usb_busses, bus);
			free(bus);
		}

		bus = tbus;
	}

	/* anything remaining in new_busses is a new bus */
	bus = new_busses;
	while (bus) {
		struct usb_bus *tbus = bus->next;
		usbi_dbg("bus %d added", bus->location);
		LIST_DEL(new_busses, bus);
		LIST_ADD(usb_busses, bus);
		changes++;
		bus = tbus;
	}

	return changes;
}

static inline int find_devices(libusb_device **dev_list, int dev_list_len,
	struct usb_bus *bus, struct usb_device **ret)
{
	struct usb_device *devices = NULL;
	struct usb_device *dev;
	int i;

	for (i = 0; i < dev_list_len; i++) {
		libusb_device *newlib_dev = dev_list[i];
		uint8_t bus_num = libusb_get_bus_number(newlib_dev);

		if (bus_num != bus->location)
			continue;

		dev = (struct usb_device *) malloc(sizeof(*dev));
		if (!dev)
			goto err;

		/* No need to reference the device now, just take the pointer. We
		 * increase the reference count later if we keep the device. */
		dev->dev = newlib_dev;

		dev->bus = bus;
		dev->devnum = libusb_get_device_address(newlib_dev);
		sprintf(dev->filename, "%03d", dev->devnum);
		LIST_ADD(devices, dev);
	}

	*ret = devices;
	return 0;

err:
	dev = devices;
	while (dev) {
		struct usb_device *tdev = dev->next;
		free(dev);
		dev = tdev;
	}
	return -ENOMEM;
}

static inline void clear_endpoint_descriptor(struct usb_endpoint_descriptor *ep)
{
	if (ep->extra)
		free(ep->extra);
}

static inline void clear_interface_descriptor(struct usb_interface_descriptor *iface)
{
	if (iface->extra)
		free(iface->extra);
	if (iface->endpoint) {
		int i;
		for (i = 0; i < iface->bNumEndpoints; i++)
			clear_endpoint_descriptor(iface->endpoint + i);
		free(iface->endpoint);
	}
}

static inline void clear_interface(struct usb_interface *iface)
{
	if (iface->altsetting) {
		int i;
		for (i = 0; i < iface->num_altsetting; i++)
			clear_interface_descriptor(iface->altsetting + i);
		free(iface->altsetting);
	}
}

static inline void clear_config_descriptor(struct usb_config_descriptor *config)
{
	if (config->extra)
		free(config->extra);
	if (config->interface) {
		int i;
		for (i = 0; i < config->bNumInterfaces; i++)
			clear_interface(config->interface + i);
		free(config->interface);
	}
}

static inline void clear_device(struct usb_device *dev)
{
	int i;
	for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
		clear_config_descriptor(dev->config + i);
}

static inline int copy_endpoint_descriptor(struct usb_endpoint_descriptor *dest,
	const struct libusb_endpoint_descriptor *src)
{
	memcpy(dest, src, USB_DT_ENDPOINT_AUDIO_SIZE);

	dest->extralen = src->extra_length;
	if (src->extra_length) {
		dest->extra = (unsigned char *) malloc(src->extra_length);
		if (!dest->extra)
			return -ENOMEM;
		memcpy(dest->extra, src->extra, src->extra_length);
	}

	return 0;
}

static inline int copy_interface_descriptor(struct usb_interface_descriptor *dest,
	const struct libusb_interface_descriptor *src)
{
	int i;
	int num_endpoints = src->bNumEndpoints;
	size_t alloc_size = sizeof(struct usb_endpoint_descriptor) * num_endpoints;

	memcpy(dest, src, USB_DT_INTERFACE_SIZE);
	dest->endpoint = (struct usb_endpoint_descriptor *) malloc(alloc_size);
	if (!dest->endpoint)
		return -ENOMEM;
	memset(dest->endpoint, 0, alloc_size);

	for (i = 0; i < num_endpoints; i++) {
		int r = copy_endpoint_descriptor(dest->endpoint + i, &src->endpoint[i]);
		if (r < 0) {
			clear_interface_descriptor(dest);
			return r;
		}
	}

	dest->extralen = src->extra_length;
	if (src->extra_length) {
		dest->extra = (unsigned char *) malloc(src->extra_length);
		if (!dest->extra) {
			clear_interface_descriptor(dest);
			return -ENOMEM;
		}
		memcpy(dest->extra, src->extra, src->extra_length);
	}

	return 0;
}

static inline int copy_interface(struct usb_interface *dest,
	const struct libusb_interface *src)
{
	int i;
	int num_altsetting = src->num_altsetting;
	size_t alloc_size = sizeof(struct usb_interface_descriptor)
		* num_altsetting;

	dest->num_altsetting = num_altsetting;
	dest->altsetting = (struct usb_interface_descriptor *) malloc(alloc_size);
	if (!dest->altsetting)
		return -ENOMEM;
	memset(dest->altsetting, 0, alloc_size);

	for (i = 0; i < num_altsetting; i++) {
		int r = copy_interface_descriptor(dest->altsetting + i,
			&src->altsetting[i]);
		if (r < 0) {
			clear_interface(dest);
			return r;
		}
	}

	return 0;
}

static inline int copy_config_descriptor(struct usb_config_descriptor *dest,
	const struct libusb_config_descriptor *src)
{
	int i;
	int num_interfaces = src->bNumInterfaces;
	size_t alloc_size = sizeof(struct usb_interface) * num_interfaces;

	memcpy(dest, src, USB_DT_CONFIG_SIZE);
	dest->interface = (struct usb_interface *) malloc(alloc_size);
	if (!dest->interface)
		return -ENOMEM;
	memset(dest->interface, 0, alloc_size);

	for (i = 0; i < num_interfaces; i++) {
		int r = copy_interface(dest->interface + i, &src->interface[i]);
		if (r < 0) {
			clear_config_descriptor(dest);
			return r;
		}
	}

	dest->extralen = src->extra_length;
	if (src->extra_length) {
		dest->extra = (unsigned char *) malloc(src->extra_length);
		if (!dest->extra) {
			clear_config_descriptor(dest);
			return -ENOMEM;
		}
		memcpy(dest->extra, src->extra, src->extra_length);
	}

	return 0;
}

static inline int initialize_device(struct usb_device *dev)
{
	libusb_device *newlib_dev = (libusb_device *) dev->dev;
	int num_configurations;
	size_t alloc_size;
	int r;
	int i;

	/* device descriptor is identical in both libs */
	r = libusb_get_device_descriptor(newlib_dev,
		(struct libusb_device_descriptor *) &dev->descriptor);
	if (r < 0) {
		usbi_err("error %d getting device descriptor", r);
		return compat_err(r);
	}

	num_configurations = dev->descriptor.bNumConfigurations;
	alloc_size = sizeof(struct usb_config_descriptor) * num_configurations;
	dev->config = (struct usb_config_descriptor *) malloc(alloc_size);
	if (!dev->config)
		return -ENOMEM;
	memset(dev->config, 0, alloc_size);

	/* even though structures are identical, we can't just use libusb-1.0's
	 * config descriptors because we have to store all configurations in
	 * a single flat memory area (libusb-1.0 provides separate allocations).
	 * we hand-copy libusb-1.0's descriptors into our own structures. */
	for (i = 0; i < num_configurations; i++) {
		struct libusb_config_descriptor *newlib_config;
		r = libusb_get_config_descriptor(newlib_dev, i, &newlib_config);
		if (r < 0) {
			clear_device(dev);
			free(dev->config);
			return compat_err(r);
		}
		r = copy_config_descriptor(dev->config + i, newlib_config);
		libusb_free_config_descriptor(newlib_config);
		if (r < 0) {
			clear_device(dev);
			free(dev->config);
			return r;
		}
	}

	/* libusb doesn't implement this and it doesn't seem that important. If
	 * someone asks for it, we can implement it in v1.1 or later. */
	dev->num_children = 0;
	dev->children = NULL;

	libusb_ref_device(newlib_dev);
	return 0;
}

static inline void free_device(struct usb_device *dev)
{
	clear_device(dev);
	libusb_unref_device((libusb_device *) dev->dev);
	free(dev);
}

inline int usb_find_devices(void)
{
	struct usb_bus *bus;
	libusb_device **dev_list;
	int dev_list_len;
	int r;
	int changes = 0;

	/* libusb-1.0 initialization might have failed, but we can't indicate
	 * this with libusb-0.1, so trap that situation here */
	if (!ctx)
		return 0;

	usbi_dbg("");
	dev_list_len = libusb_get_device_list(ctx, &dev_list);
	if (dev_list_len < 0)
		return compat_err(dev_list_len);

	for (bus = usb_busses; bus; bus = bus->next) {
		struct usb_device *new_devices = NULL;
		struct usb_device *dev;

		r = find_devices(dev_list, dev_list_len, bus, &new_devices);
		if (r < 0) {
			libusb_free_device_list(dev_list, 1);
			return r;
		}

		/* walk through the devices we already know about, removing duplicates
		 * from the new list. if we do not find it in the new list, the device
		 * has been removed. */
		dev = bus->devices;
		while (dev) {
			int found = 0;
			struct usb_device *tdev = dev->next;
			struct usb_device *ndev = new_devices;

			while (ndev) {
				if (ndev->devnum == dev->devnum) {
					LIST_DEL(new_devices, ndev);
					free(ndev);
					found = 1;
					break;
				}
				ndev = ndev->next;
			}

			if (!found) {
				usbi_dbg("device %d.%d removed",
					dev->bus->location, dev->devnum);
				LIST_DEL(bus->devices, dev);
				free_device(dev);
				changes++;
			}

			dev = tdev;
		}

		/* anything left in new_devices is a new device */
		dev = new_devices;
		while (dev) {
			struct usb_device *tdev = dev->next;
			r = initialize_device(dev);
			if (r < 0) {
				usbi_err("couldn't initialize device %d.%d (error %d)",
					dev->bus->location, dev->devnum, r);
				dev = tdev;
				continue;
			}
			usbi_dbg("device %d.%d added", dev->bus->location, dev->devnum);
			LIST_DEL(new_devices, dev);
			LIST_ADD(bus->devices, dev);
			changes++;
			dev = tdev;
		}
	}

	libusb_free_device_list(dev_list, 1);
	return changes;
}

inline usb_dev_handle *usb_open(struct usb_device *dev)
{
	int r;
	usbi_dbg("");

	usb_dev_handle *udev = (usb_dev_handle *) malloc(sizeof(*udev));
	if (!udev)
		return NULL;

	r = libusb_open((libusb_device *) dev->dev, &udev->handle);
	if (r < 0) {
		if (r == LIBUSB_ERROR_ACCESS) {
			usbi_info("Device open failed due to a permission denied error.");
			usbi_info("libusb requires write access to USB device nodes.");
		}
		usbi_err("could not open device, error %d", r);
		free(udev);
		errno = libusb_to_errno(r);
		return NULL;
	}

	udev->last_claimed_interface = -1;
	udev->device = dev;
	return udev;
}

inline int usb_close(usb_dev_handle *dev)
{
	usbi_dbg("");
	libusb_close(dev->handle);
	free(dev);
	return 0;
}

inline int usb_reset(usb_dev_handle *dev)
{
	usbi_dbg("");
	return compat_err(libusb_reset_device(dev->handle));
}

static inline int usb_bulk_io(usb_dev_handle *dev, int ep, char *bytes,
	int size, int timeout)
{
	int actual_length;
	int r;
	usbi_dbg("endpoint %x size %d timeout %d", ep, size, timeout);
	r = libusb_bulk_transfer(dev->handle, ep & 0xff, (unsigned char *) bytes, size,
		&actual_length, timeout);

	/* if we timed out but did transfer some data, report as successful short
	 * read. FIXME: is this how libusb-0.1 works? */
	if (r == 0 || (r == LIBUSB_ERROR_TIMEOUT && actual_length > 0))
		return actual_length;

	return compat_err(r);
}

inline int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes,
	int size, int timeout)
{
	if (!(ep & USB_ENDPOINT_IN)) {
		/* libusb-0.1 will strangely fix up a read request from endpoint
		 * 0x01 to be from endpoint 0x81. do the same thing here, but
		 * warn about this silly behaviour. */
		usbi_warn("endpoint %x is missing IN direction bit, fixing");
		ep |= USB_ENDPOINT_IN;
	}

	return usb_bulk_io(dev, ep, bytes, size, timeout);
}

inline int usb_bulk_write(usb_dev_handle *dev, int ep, const char *bytes,
	int size, int timeout)
{
	if (ep & USB_ENDPOINT_IN) {
		/* libusb-0.1 on BSD strangely fix up a write request to endpoint
		 * 0x81 to be to endpoint 0x01. do the same thing here, but
		 * warn about this silly behaviour. */
		usbi_warn("endpoint %x has excessive IN direction bit, fixing");
		ep &= ~USB_ENDPOINT_IN;
	}

	return usb_bulk_io(dev, ep, (char *)bytes, size, timeout);
}

inline int usb_control_msg(usb_dev_handle *dev, int bmRequestType,
	int bRequest, int wValue, int wIndex, char *bytes, int size, int timeout)
{
	int r;
	usbi_dbg("RQT=%x RQ=%x V=%x I=%x len=%d timeout=%d", bmRequestType,
		bRequest, wValue, wIndex, size, timeout);

	r = libusb_control_transfer(dev->handle, bmRequestType & 0xff,
		bRequest & 0xff, wValue & 0xffff, wIndex & 0xffff, (unsigned char *) bytes, size & 0xffff,
		timeout);

	if (r >= 0)
		return r;

	return compat_err(r);
}

#endif
#endif  // AML_USB_COMPAT_H
