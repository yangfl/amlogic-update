#include <algorithm>
#include <cstring>

#include "AmlLibusb.hpp"
#include "../Amldbglog/Amldbglog.h"
#include "../defs.h"


int IOCTL_READ_MEM_Handler (usbDevIoCtrl ctrl) {
  if (!ctrl.in_buf || !ctrl.out_buf || ctrl.in_len != 4 || !ctrl.out_len) {
    return 0;
  }

  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_READ_CTRL, SHORT_AT(ctrl.in_buf, 2),
                            SHORT_AT(ctrl.in_buf, 0), ctrl.out_buf,
                            std::min(ctrl.out_len, 64u), 50000);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("IOCTL_READ_MEM_Handler ret=%d error_msg=%s\n", ret, usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_WRITE_MEM_Handler (usbDevIoCtrl ctrl) {
  if (!ctrl.in_buf || ctrl.in_len <= 2) {
    aml_printf("f(%s)L%d, ivlaid, in_buf=0x, in_len=%d\n", "AmlLibusb/AmlLibusb.cpp",
               47, ctrl.in_buf, ctrl.in_len);
    return 0;
  }

  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_WRITE_CTRL, SHORT_AT(ctrl.in_buf, 2),
                            SHORT_AT(ctrl.in_buf, 0), ctrl.in_buf + 4,
                            std::min(ctrl.in_len - 4, 64u), 50000);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("IOCTL_WRITE_MEM_Handler ret=%d error_msg=%d\n", ret, usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_READ_AUX_REG_Handler (usbDevIoCtrl ctrl) {
  return 0;
}


int IOCTL_WRITE_AUX_REG_Handler (usbDevIoCtrl ctrl) {
  return 0;
}


int IOCTL_FILL_MEM_Handler (usbDevIoCtrl ctrl) {
  return 0;
}


int IOCTL_MODIFY_MEM_Handler (usbDevIoCtrl ctrl) {
  return 0;
}


int IOCTL_RUN_IN_ADDR_Handler (usbDevIoCtrl ctrl) {
  if (!ctrl.in_buf || ctrl.in_len != 4) {
    return 0;
  }

  ctrl.in_buf[0] |= 0x10;
  usb_control_msg(ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                  AML_LIBUSB_REQ_RUN_ADDR, SHORT_AT(ctrl.in_buf, 2),
                  SHORT_AT(ctrl.in_buf, 0), ctrl.in_buf, 4, 50000);
  return 1LL;
}


int IOCTL_DO_LARGE_MEM_Handler (usbDevIoCtrl ctrl, int readOrWrite) {
  if (!ctrl.in_buf || ctrl.in_len <= 0xF) {
    return 0;
  }

  unsigned short value = 0;
  if (ctrl.in_len > 16) {
    value = SHORT_AT(ctrl.in_buf, 64);
  }
  if (value == 0 || value > 4096) {
    value = 4096;
  }
  unsigned long len = LONG_AT(ctrl.in_buf, 4);
  unsigned short index = (unsigned short) (value + len - 1) / value;
  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                            readOrWrite ? AML_LIBUSB_REQ_READ_MEM
                                        : AML_LIBUSB_REQ_WRITE_MEM, value, index,
                            ctrl.in_buf, 16, 50000);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("[%s],value=%x,index=%x,len=%d,ret=%d error_msg=%s\n",
               readOrWrite ? "read" : "write", value, index, len, ret, usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_IDENTIFY_HOST_Handler (usbDevIoCtrl ctrl) {
  if (ctrl.out_buf && ctrl.out_len <= 8) {
    int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
                              AML_LIBUSB_REQ_IDENTIFY, 0, 0, ctrl.out_buf, ctrl.out_len,
                              5000);
    *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
    if (ret < 0) {
      aml_printf("IOCTL_IDENTIFY_HOST_Handler ret=%d error_msg=%s\n", ret,
                 usb_strerror());
    }
    return ret >= 0;
  } else {
    aml_printf("identify,%p,%d(max 8)\n", ctrl.out_buf, ctrl.out_len);
    return 0;
  }
}


int IOCTL_TPL_CMD_Handler (usbDevIoCtrl ctrl) {
  if (!ctrl.in_buf || ctrl.in_len != 68) {
    return 0;
  }

  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_TPL_CMD, SHORT_AT(ctrl.in_buf, 64),
                            SHORT_AT(ctrl.in_buf, 66), ctrl.in_buf, 64, 50000);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("IOCTL_TPL_CMD_Handler ret=%d,tpl_cmd=%s error_msg=%s\n", ret, ctrl.in_buf,
               usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_TPL_STATUS_Handler (usbDevIoCtrl ctrl) {
  return IOCTL_TPL_STATUS_Handler_Ex(ctrl, 60);
}


int IOCTL_TPL_STATUS_Handler_Ex (usbDevIoCtrl ctrl, unsigned int timeout) {
  if (!ctrl.in_buf || !ctrl.out_buf || ctrl.in_len != 4 || ctrl.out_len != 64) {
    return 0;
  }

  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_TPL_STATUS, SHORT_AT(ctrl.in_buf, 0),
                            SHORT_AT(ctrl.in_buf, 2), ctrl.out_buf, 64, 1000 * timeout);
  if (ret < 0) {
    aml_printf("IOCTL_TPL_STATUS_Handler ret=%d error_msg=%s\n", ret, usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_WRITE_MEDIA_Handler (usbDevIoCtrl ctrl, unsigned int timeout) {
  if (ctrl.in_buf && ctrl.in_len > 0x1F) {
    *((short *) ctrl.in_buf + 8) = 239;
    *((short *) ctrl.in_buf + 9) = 256;
    int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                              AML_LIBUSB_REQ_WRITE_MEDIA, 1, 0xFFFF, ctrl.in_buf, 32,
                              timeout);
    *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
    if (ret < 0) {
      aml_printf("[AmlLibUsb]:");
      aml_printf(
          "IOCTL_WRITE_MEDIA_Handler,value=%x,index=%x,len=%d,ret=%d error_msg=%s\n", 1,
          0xFFFFLL, 32LL, ret, usb_strerror());
    }
    return ret >= 0;
  } else {
    aml_printf("[AmlLibUsb]:");
    aml_printf("in_buf=%p, in_len=%d\n", ctrl.in_buf, ctrl.in_len);
    return 0;
  }
}


int IOCTL_READ_MEDIA_Handler (usbDevIoCtrl ctrl, unsigned int timeout) {
  if (!ctrl.in_buf || ctrl.in_len <= 0xF) {
    return 0;
  }

  int value = 0;
  if (ctrl.in_len > 32) {
    value = *((short *) ctrl.in_buf + 32);
  }
  if (!value || value > 0x1000) {
    value = 4096;
  }
  unsigned long len = LONG_AT(ctrl.in_buf, 4);
  unsigned int index = (unsigned int) (value + len - 1) / value;
  int ret = usb_control_msg(ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_READ_MEDIA, value, index, ctrl.in_buf, 16,
                            timeout);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("IOCTL_READ_MEDIA_Handler,value=%x,index=%x,len=%d,ret=%d error_msg=%s\n",
               value, index, len, ret, usb_strerror());
  }
  return ret >= 0;
}


int IOCTL_BULK_CMD_Handler (usbDevIoCtrl ctrl, unsigned int a1, unsigned int request) {
  if (!ctrl.in_buf || ctrl.in_len != 68) {
    return 0;
  }

  int ret = usb_control_msg(ctrl.handle, 64, request, 0, 2, ctrl.in_buf, 64, 50000);
  *ctrl.p_in_data_size = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("AM_REQ_BULK_CMD_Handler ret=%d,blkcmd=%s error_msg=%s\n", ret,
               ctrl.in_buf, usb_strerror());
  }
  return ret >= 0;
}


int usbDeviceIoControl (AmlUsbDrv *drv, unsigned int control_code, void *in_buf,
                        unsigned int in_len, void *out_buf, unsigned int out_len,
                        unsigned int *in_data_size, unsigned int *out_data_size) {
  if (drv->device == (struct usb_device *) -1) {
    aml_printf("hDevice == INVALID_HANDLE_VALUE\n");
    return 0;
  }

  usbDevIoCtrl ctrl = {.handle = drv
      ->handle, .in_buf = (char *) in_buf, .out_buf = (char *) out_buf, .in_len = in_len, .out_len = out_len, .p_in_data_size = in_data_size, .p_out_data_size = out_data_size};
  usbDevIo v18 = {.a = 1, .b = 0, .device = drv->device};
  int readOrWrite = 0;

  switch (control_code) {
    case 0x80002000:
      return IOCTL_READ_MEM_Handler(ctrl);
    case 0x80002004:
      return IOCTL_WRITE_MEM_Handler(ctrl);
    case 0x80002008:
      return IOCTL_FILL_MEM_Handler(ctrl);
    case 0x8000200C:
      return IOCTL_RUN_IN_ADDR_Handler(ctrl);
    case 0x80002010:
      readOrWrite = 1;
      [[fallthrough]];
    case 0x80002014:
      return IOCTL_DO_LARGE_MEM_Handler(ctrl, readOrWrite);
    case 0x80002018:
      return IOCTL_READ_AUX_REG_Handler(ctrl);
    case 0x8000201C:
      return IOCTL_WRITE_AUX_REG_Handler(ctrl);
    case 0x80002020:
      return IOCTL_MODIFY_MEM_Handler(ctrl);
    case 0x80002024:
      return IOCTL_IDENTIFY_HOST_Handler(ctrl);
    case 0x80002040:
      return IOCTL_TPL_CMD_Handler(ctrl);
    case 0x80002044:
      return IOCTL_TPL_STATUS_Handler(ctrl);
    default:
      return 0;
  }
}


int usbDeviceIoControlEx (AmlUsbDrv *drv, unsigned int controlCode, void *in_buf,
                          unsigned int in_len, void *out_buf, unsigned int out_len,
                          unsigned int *in_data_size, unsigned int *out_data_size,
                          unsigned int timeout) {
  if (drv->device == (struct usb_device *) -1) {
    aml_printf("hDevice == INVALID_HANDLE_VALUE\n");
    return 0;
  }

  usbDevIoCtrl ctrl = {.handle = drv
      ->handle, .in_buf = (char *) in_buf, .out_buf = (char *) out_buf, .in_len = in_len, .out_len = out_len, .p_in_data_size = in_data_size, .p_out_data_size = out_data_size};
  usbDevIo v18 = {.a = 1, .b = 0, .device = drv->device};

  switch (controlCode) {
    case 0x80002004:
      return IOCTL_WRITE_MEM_Handler(ctrl);
    case 0x80002044:
      return IOCTL_TPL_STATUS_Handler_Ex(ctrl, timeout);
    case 0x80002048:
      return IOCTL_WRITE_MEDIA_Handler(ctrl, timeout);
    case 0x8000204C:
      return IOCTL_READ_MEDIA_Handler(ctrl, timeout);
    case 0x80002050:
      return IOCTL_BULK_CMD_Handler(ctrl, timeout, 0x34);
    case 0x80002054:
      return IOCTL_BULK_CMD_Handler(ctrl, timeout, 0x35);
    default:
      aml_printf("unknown control code 0x%x\n", controlCode);
      return 0;
  }
}


int usbReadFile (AmlUsbDrv *drv, void *buf, unsigned int len, unsigned int *read) {
  if (drv->device == (struct usb_device *) -1) {
    return 0;
  }

  int ret = usb_bulk_read(drv->handle, drv->read_ep, (char *) buf, len, 90000);
  *read = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("usbReadFile len=%d,ret=%d error_msg=%s\n", len, ret, usb_strerror());
  }
  return ret >= 0;
}


int usbWriteFile (AmlUsbDrv *drv, const void *buf, unsigned int len, unsigned int *read) {
  if (drv->device == (struct usb_device *) -1) {
    return 0;
  }

  int ret = usb_bulk_write(drv->handle, drv->write_ep, (char *) buf, len, 50000);
  *read = (unsigned int) std::max(0, ret);
  if (ret < 0) {
    aml_printf("usbWriteFile len=%d,ret=%d error_msg=%s\n", len, ret, usb_strerror());
  }
  return ret >= 0;
}


int OpenUsbDevice (AmlUsbDrv *drv) {
  if (drv && drv->device && !drv->handle) {
    drv->handle = usb_open(drv->device);
    drv->read_ep = (char) 0x81;
    drv->write_ep = 2;
    return 1;
  } else {
    aml_printf("%s L%d bug %#x %#x %#x\n", "OpenUsbDevice", 572, drv,
               drv ? drv->device : nullptr, drv ? drv->handle : nullptr);
    return 0;
  }
}


int CloseUsbDevice (AmlUsbDrv *drv) {
  if (drv && drv->device) {
    usb_close(drv->handle);
    drv->handle = nullptr;
    return 1;
  } else {
    aml_printf("%s L%d bug %#x %#x %#x\n", "CloseUsbDevice", 592, drv,
               drv ? drv->device : nullptr, drv ? drv->handle : nullptr);
    return 0;
  }
}

int ResetDev (AmlUsbDrv *drv) {
  if (drv->device == (struct usb_device *) -1) {
    return 0;
  }

  aml_printf("Reset WorldCup Device\n");
  return usb_reset(drv->handle);
}


// Control Read/Write
// Read = 1
int Aml_Libusb_Ctrl_RdWr (void *device, unsigned int offset, char *buf, unsigned int len,
                          unsigned int readOrWrite, unsigned int timeout) {
  struct AmlUsbDrv drv = {.device = (struct usb_device *) device};

  if (OpenUsbDevice(&drv) != 1) {
    aml_printf("Fail in open dev\n");
    return -647;
  }

  int processedData = 0;
  while (processedData < len) {
    int requestLen = std::min(len - processedData, 64u);
    usb_control_msg(drv.handle,
                    readOrWrite ? USB_ENDPOINT_IN | USB_TYPE_VENDOR : USB_TYPE_VENDOR,
                    readOrWrite ? AML_LIBUSB_REQ_READ_CTRL : AML_LIBUSB_REQ_WRITE_CTRL,
                    offset >> 16, offset, buf, requestLen, timeout);
    processedData += requestLen;
    buf += requestLen;
    offset += requestLen;
  }
  CloseUsbDevice(&drv);
  return 0;
}


int Aml_Libusb_Password (void *device, char *buf, int size, int timeout) {
  struct AmlUsbDrv drv = {.device = (struct usb_device *) device};

  if (OpenUsbDevice(&drv) != 1) {
    aml_printf("Fail in open dev\n");
    return -698;
  }

  if (size > 64) {
    aml_printf("f(%s)size(%d) too large, cannot support it.\n", "Aml_Libusb_Password",
               size);
    return -705;
  }

  int bufPtr = 0;
  int value = 0;
  while (bufPtr < size) {
    value += buf[bufPtr++];
  }
  int result = usb_control_msg(drv.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
                               AML_LIBUSB_REQ_PASSWORD, value, 0, buf, size, timeout);
  CloseUsbDevice(&drv);
  return result;
}


int Aml_Libusb_get_chipinfo (void *device, char *buf, int size, int index, int timeout) {
  struct AmlUsbDrv drv = {.device = (struct usb_device *) device};

  if (OpenUsbDevice(&drv) != 1) {
    aml_printf("Fail in open dev\n");
    return -750;
  }

  if (size > 64) {
    aml_printf("f(%s)size(%d) too large, cannot support it.\n", "Aml_Libusb_get_chipinfo",
               (unsigned int) size);
    return -757;
  }

  int ret = usb_control_msg(drv.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
                            AML_LIBUSB_REQ_CHIP_INFO, 0, index, buf, size, timeout);
  CloseUsbDevice(&drv);
  return ret;
}
