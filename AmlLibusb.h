#ifndef AML_LIB_USB_H
#define AML_LIB_USB_H

#include "usbcompat.h"


#define AM_EP_IN 0x81
#define AM_EP_OUT 0x02


struct _OVERLAPPED {
  uintptr_t ternal;
  uintptr_t InternalHigh;
  union {
    struct {
      uint32_t Offset;
      uint32_t OffsetHigh;
    } DUMMYSTRUCTNAME;
    void *Pointer;
  } DUMMYUNIONNAME;
  void *hEvent;
};


struct AmlUsbDrv {
  struct usb_device *device;
  usb_dev_handle *handle;
  unsigned char read_ep;
  unsigned char write_ep;
};


#define CTL_CODE(DeviceType,Function,Method,Access) ((unsigned int) ( \
  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)))

#define METHOD_BUFFERED       0
#define METHOD_IN_DIRECT      1
#define METHOD_OUT_DIRECT     2
#define METHOD_NEITHER        3

#define METHOD_DIRECT_TO_HARDWARE METHOD_IN_DIRECT
#define METHOD_DIRECT_FROM_HARDWARE METHOD_OUT_DIRECT

#define FILE_ANY_ACCESS         0
#define FILE_SPECIAL_ACCESS     (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS        (0x0001)
#define FILE_WRITE_ACCESS       (0x0002)


#define IOCTL_BASE 0x00008000

#define IOCTL_READ_MEM          CTL_CODE(IOCTL_BASE, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEM         CTL_CODE(IOCTL_BASE, 0x0801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FILL_MEM          CTL_CODE(IOCTL_BASE, 0x0802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RUN_IN_ADDR       CTL_CODE(IOCTL_BASE, 0x0803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_LARGE_MEM    CTL_CODE(IOCTL_BASE, 0x0804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_LARGE_MEM   CTL_CODE(IOCTL_BASE, 0x0805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_AUX_REG      CTL_CODE(IOCTL_BASE, 0x0806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_AUX_REG     CTL_CODE(IOCTL_BASE, 0x0807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MODIFY_MEM        CTL_CODE(IOCTL_BASE, 0x0808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IDENTIFY_HOST     CTL_CODE(IOCTL_BASE, 0x0809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TPL_CMD           CTL_CODE(IOCTL_BASE, 0x0810, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TPL_STATUS        CTL_CODE(IOCTL_BASE, 0x0811, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEDIA       CTL_CODE(IOCTL_BASE, 0x0812, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_MEDIA        CTL_CODE(IOCTL_BASE, 0x0813, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BULK_CMD_1        CTL_CODE(IOCTL_BASE, 0x0814, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BULK_CMD_2        CTL_CODE(IOCTL_BASE, 0x0815, METHOD_BUFFERED, FILE_ANY_ACCESS)


int usbDeviceIoControl (
  struct AmlUsbDrv *drv, unsigned int dwIoControlCode, void *lpInBuffer,
  unsigned int nInBufferSize, void *lpOutBuffer, unsigned int nOutBufferSize,
  unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped);
int usbDeviceIoControlEx (
  struct AmlUsbDrv *drv, unsigned int dwIoControlCode, void *lpInBuffer,
  unsigned int nInBufferSize, void *lpOutBuffer, unsigned int nOutBufferSize,
  unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped,
  unsigned int timeout);
int usbReadFile (
  struct AmlUsbDrv *drv, void *lpOutBuffer, unsigned int nOutBufferSize,
  unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped);
int usbWriteFile (
  struct AmlUsbDrv *drv, const void *lpInBuffer, unsigned int nInBufferSize,
  unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped);


int OpenUsbDevice (struct AmlUsbDrv *drv);
int CloseUsbDevice (struct AmlUsbDrv *drv);
int ResetDev (struct AmlUsbDrv *drv);


int Aml_Libusb_Ctrl_RdWr (
  void *dev, uint32_t addr, char *buf, int len, int doRead, int timeout);
int Aml_Libusb_Password (void *dev, char *buf, int size, int timeout);
int Aml_Libusb_get_chipinfo (
  void *dev, char *buf, int size, int index, int timeout);

int Aml_Libusb_bl2_test (void *dev, int timeout, int argc, const char **argv);
int Aml_Libusb_bl2_boot (void *dev, int timeout, int argc, const char **argv);


#endif  // AML_LIB_USB_H
