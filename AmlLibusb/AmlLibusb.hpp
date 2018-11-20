#ifndef AML_LIB_USB_H
#define AML_LIB_USB_H

#include <usb.h>


struct usbDevIoCtrl {
  usb_dev_handle *handle;
  char *in_buf;  // host to device
  char *out_buf;  // device to host
  unsigned int in_len;
  unsigned int out_len;
  unsigned int *p_in_data_size;
  unsigned int *p_out_data_size;
};


struct usbDevIo {
  void *p0;
  void *p1;
  void *p2;
  int a;
  int b;
  void *p5;
  struct usb_device *device;
};


struct AmlUsbDrv {
  struct usb_device *device;
  usb_dev_handle *handle;
  char read_ep;
  char write_ep;
};


enum AmlUsbRequest {
  AML_LIBUSB_REQ_WRITE_CTRL = 0x01,
  AML_LIBUSB_REQ_READ_CTRL = 0x02,
  AML_LIBUSB_REQ_RUN_ADDR = 0x05,
  // MODIFY_MEM
  // FILL_MEM
  // WRITE_AUX_ARG
  // READ_AUX_ARG
  AML_LIBUSB_REQ_WRITE_MEM = 0x11,
  AML_LIBUSB_REQ_READ_MEM = 0x12,
  AML_LIBUSB_REQ_IDENTIFY = 0x20,
  AML_LIBUSB_REQ_TPL_CMD = 0x30,
  AML_LIBUSB_REQ_TPL_STATUS = 0x31,
  AML_LIBUSB_REQ_WRITE_MEDIA = 0x32,
  AML_LIBUSB_REQ_READ_MEDIA = 0x33,
  // 34
  AML_LIBUSB_REQ_PASSWORD = 0x35,
  AML_LIBUSB_REQ_CHIP_INFO = 0x40,
};


int IOCTL_READ_MEM_Handler (usbDevIoCtrl ctrl);
int IOCTL_WRITE_MEM_Handler (usbDevIoCtrl ctrl);
int IOCTL_READ_AUX_REG_Handler (usbDevIoCtrl ctrl);
int IOCTL_WRITE_AUX_REG_Handler (usbDevIoCtrl ctrl);
int IOCTL_FILL_MEM_Handler (usbDevIoCtrl ctrl);
int IOCTL_MODIFY_MEM_Handler (usbDevIoCtrl ctrl);
int IOCTL_RUN_IN_ADDR_Handler (usbDevIoCtrl ctrl);
int IOCTL_DO_LARGE_MEM_Handler (usbDevIoCtrl ctrl, int readOrWrite);
int IOCTL_IDENTIFY_HOST_Handler (usbDevIoCtrl ctrl);
int IOCTL_TPL_CMD_Handler (usbDevIoCtrl ctrl);
int IOCTL_TPL_STATUS_Handler (usbDevIoCtrl ctrl);
int IOCTL_TPL_STATUS_Handler_Ex(usbDevIoCtrl ctrl, unsigned int timeout);
int IOCTL_WRITE_MEDIA_Handler (usbDevIoCtrl ctrl, unsigned int timeout);
int IOCTL_READ_MEDIA_Handler (usbDevIoCtrl ctrl, unsigned int timeout);
int IOCTL_BULK_CMD_Handler (usbDevIoCtrl ctrl, unsigned int a1, unsigned int request);
int usbDeviceIoControl (
  AmlUsbDrv *drv, unsigned int control_code, void *in_buf, unsigned int in_len,
  void *out_buf, unsigned int out_len, unsigned int *in_data_size,
  unsigned int *out_data_size);
int usbDeviceIoControlEx (
  AmlUsbDrv *drv, unsigned int controlCode, void *in_buf, unsigned int in_len,
  void *out_buf, unsigned int out_len, unsigned int *in_data_size,
  unsigned int *out_data_size, unsigned int timeout);
int usbReadFile (AmlUsbDrv *drv, void *buf, unsigned int len, unsigned int *read);
int usbWriteFile (AmlUsbDrv *drv, const void *buf, unsigned int len, unsigned int *read);
int OpenUsbDevice (AmlUsbDrv *drv);
int CloseUsbDevice (AmlUsbDrv *drv);
int ResetDev (AmlUsbDrv *drv);
int Aml_Libusb_Ctrl_RdWr (
  void *device, unsigned int offset, char *buf, unsigned int len,
  unsigned int readOrWrite, unsigned int timeout);
int Aml_Libusb_Password (void *device, char *buf, int size, int timeout);
int Aml_Libusb_get_chipinfo (void *device, char *buf, int size, int index, int timeout);

#endif  // AML_LIB_USB_H
