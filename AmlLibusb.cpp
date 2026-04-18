#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <usb.h>

#include "checksum.h"
#include "defs.h"

#include "Amldbglog.h"
#include "AmlLibusb.h"


#define INVALID_HANDLE_VALUE ((void *) -1)


#define AM_REQ_WRITE_MEM        0x01
#define AM_REQ_READ_MEM         0x02
#define AM_REQ_FILL_MEM         0x03
#define AM_REQ_MODIFY_MEM       0x04
#define AM_REQ_RUN_IN_ADDR      0x05
#define AM_REQ_WRITE_AUX        0x06
#define AM_REQ_READ_AUX         0x07
#define AM_REQ_WR_LARGE_MEM     0x11
#define AM_REQ_RD_LARGE_MEM     0x12
#define AM_REQ_IDENTIFY_HOST    0x20
#define AM_REQ_TPL_CMD          0x30
#define AM_REQ_TPL_STAT         0x31
#define AM_REQ_WRITE_MEDIA      0x32
#define AM_REQ_READ_MEDIA       0x33
#define AM_REQ_PASSWORD         0x35
#define AM_REQ_NOP              0x36
#define AM_REQ_CHIP_INFO        0x40
#define AM_REQ_START            0x50
#define AM_REQ_CMD4             0x60

#define AM_RUNNING_FLAGS_KEEP_POWER_ON  0x10


struct usbDevIoCtrl {
  usb_dev_handle *handle;
  char *in_buf;  // host to device
  char *out_buf;  // device to host
  unsigned int in_len;
  unsigned int out_len;
  unsigned int *p_bytes_returned;
  struct _OVERLAPPED *p_overlapped;
};


static int IOCTL_READ_MEM_Handler (struct usbDevIoCtrl ctrl) {
  if (ctrl.in_buf == NULL || ctrl.out_buf == NULL || ctrl.in_len != 4 ||
      ctrl.out_buf != 0) {
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, AM_REQ_READ_MEM,
    GET_U16(&ctrl.in_buf[2]), GET_U16(&ctrl.in_buf[0]), ctrl.out_buf,
    aml_min(ctrl.out_len, 64u), 50000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "IOCTL_READ_MEM_Handler ret=%d error_msg=%s\n", ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_WRITE_MEM_Handler (struct usbDevIoCtrl ctrl) {
  if (ctrl.in_buf == NULL || ctrl.in_len <= 2) {
    aml_printf(
#ifndef BUGFIX
      "f(%s)L%d, ivlaid, in_buf=0x, in_len=%d\n",
#else
      "f(%s)L%d, ivlaid, in_buf=0x%p, in_len=%d\n",
#endif
      "AmlLibusb.cpp", 45, ctrl.in_buf, ctrl.in_len);
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_WRITE_MEM,
    GET_U16(&ctrl.in_buf[2]), GET_U16(&ctrl.in_buf[0]), ctrl.in_buf + 4,
    aml_min(ctrl.in_len - 4, 64u), 50000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
#ifndef BUGFIX
      "IOCTL_WRITE_MEM_Handler ret=%d error_msg=%d\n",
#else
      "IOCTL_WRITE_MEM_Handler ret=%d error_msg=%s\n",
#endif
      ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_READ_AUX_REG_Handler (struct usbDevIoCtrl ctrl) {
  (void) ctrl;

  return 0;
}


static int IOCTL_WRITE_AUX_REG_Handler (struct usbDevIoCtrl ctrl) {
  (void) ctrl;

  return 0;
}


static int IOCTL_FILL_MEM_Handler (struct usbDevIoCtrl ctrl) {
  (void) ctrl;

  return 0;
}


static int IOCTL_MODIFY_MEM_Handler (struct usbDevIoCtrl ctrl) {
  (void) ctrl;

  return 0;
}


static int IOCTL_RUN_IN_ADDR_Handler (struct usbDevIoCtrl ctrl) {
  if (ctrl.in_buf == NULL || ctrl.in_len != 4) {
    return 0;
  }

  uint16_t lo = GET_U16(&ctrl.in_buf[0]);
  ctrl.in_buf[0] |= 0x10;

  usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_RUN_IN_ADDR,
    GET_U16(&ctrl.in_buf[2]), lo, ctrl.in_buf, 4, 50000);

  return 1;
}


static int IOCTL_DO_LARGE_MEM_Handler (struct usbDevIoCtrl ctrl, int doRead) {
  if (ctrl.in_buf == NULL || ctrl.in_len < 16) {
    return 0;
  }

  uint16_t bulkSize = 0;
  if (ctrl.in_len > 16) {
    bulkSize = GET_U16(&ctrl.in_buf[64]);
  }
  if (bulkSize == 0 || bulkSize > 4096) {
    bulkSize = 4096;
  }
  uint32_t len = GET_U32(&ctrl.in_buf[4]);
  uint16_t cnt = (len + bulkSize - 1) / bulkSize;

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
    doRead ? AM_REQ_RD_LARGE_MEM : AM_REQ_WR_LARGE_MEM,
    bulkSize, cnt, ctrl.in_buf, 16, 50000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "[%s],value=%x,index=%x,len=%d,ret=%d error_msg=%s\n",
      doRead ? "read" : "write", bulkSize, cnt, len, ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_IDENTIFY_HOST_Handler (struct usbDevIoCtrl ctrl) {
  if (ctrl.out_buf == NULL || ctrl.out_len > 8) {
    aml_printf("identify,%p,%d(max 8)\n", ctrl.out_buf, ctrl.out_len);
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, AM_REQ_IDENTIFY_HOST,
    0, 0, ctrl.out_buf, ctrl.out_len, 5000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "IOCTL_IDENTIFY_HOST_Handler ret=%d error_msg=%s\n", ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_TPL_CMD_Handler (struct usbDevIoCtrl ctrl) {
  if (ctrl.in_buf == NULL || ctrl.in_len != 68) {
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_TPL_CMD,
    GET_U16(&ctrl.in_buf[64]), GET_U16(&ctrl.in_buf[66]), ctrl.in_buf, 64,
    50000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "IOCTL_TPL_CMD_Handler ret=%d,tpl_cmd=%s error_msg=%s\n", ret,
      ctrl.in_buf, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_TPL_STATUS_Handler_Ex (
    struct usbDevIoCtrl ctrl, unsigned int timeout) {
  if (ctrl.in_buf == NULL || ctrl.out_buf == NULL || ctrl.in_len != 4 ||
      ctrl.out_len != 64) {
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, AM_REQ_TPL_STAT,
    GET_U16(&ctrl.in_buf[0]), GET_U16(&ctrl.in_buf[2]), ctrl.out_buf, 64,
    1000 * timeout);

  if (ret < 0) {
    aml_printf(
      "IOCTL_TPL_STATUS_Handler ret=%d error_msg=%s\n", ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_TPL_STATUS_Handler (struct usbDevIoCtrl ctrl) {
  return IOCTL_TPL_STATUS_Handler_Ex(ctrl, 60);
}


static int IOCTL_WRITE_MEDIA_Handler (struct usbDevIoCtrl ctrl, unsigned int timeout) {
  if (ctrl.in_buf == NULL || ctrl.in_len < 32) {
    aml_printf("[LUSB]in_buf=%p, in_len=%d\n", ctrl.in_buf, ctrl.in_len);
    return 0;
  }

  SET_U32(&ctrl.in_buf[16], 0x010000EF);

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_WRITE_MEDIA,
    1, 0xFFFF, ctrl.in_buf, 32, timeout);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "[LUSB]IOCTL_WRITE_MEDIA_Handler,value=%x,index=%x,len=%d,ret=%d error_msg=%s\n",
      1, 0xFFFF, 32, ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_READ_MEDIA_Handler (struct usbDevIoCtrl ctrl, unsigned int timeout) {
  if (ctrl.in_buf == NULL || ctrl.in_len < 16) {
    return 0;
  }

  uint16_t bulkSize = 0;
  if (ctrl.in_len > 16) {
    bulkSize = GET_U16(&ctrl.in_buf[64]);
  }
  if (bulkSize == 0 || bulkSize > 4096) {
    bulkSize = 4096;
  }
  uint32_t len = GET_U32(&ctrl.in_buf[4]);
  uint16_t cnt = (len + bulkSize - 1) / bulkSize;

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, AM_REQ_READ_MEDIA,
    bulkSize, cnt, ctrl.in_buf, 16, timeout);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "IOCTL_READ_MEDIA_Handler,value=%x,index=%x,len=%d,ret=%d error_msg=%s\n",
      bulkSize, cnt, len, ret, usb_strerror());
    return 0;
  }
  return 1;
}


static int IOCTL_BULK_CMD_Handler (
    struct usbDevIoCtrl ctrl, unsigned int timeout, unsigned int request) {
  (void) timeout;

  if (ctrl.in_buf == NULL || ctrl.in_len != 68) {
    return 0;
  }

  int ret = usb_control_msg(
    ctrl.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
    request, 0, 2, ctrl.in_buf, 64, 50000);

  *ctrl.p_bytes_returned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "AM_REQ_BULK_CMD_Handler ret=%d,blkcmd=%s error_msg=%s\n", ret,
      ctrl.in_buf, usb_strerror());
    return 0;
  }
  return 1;
}


int usbDeviceIoControl (
    struct AmlUsbDrv *drv, unsigned int dwIoControlCode, void *lpInBuffer,
    unsigned int nInBufferSize, void *lpOutBuffer, unsigned int nOutBufferSize,
    unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped) {
  struct usb_device *hDevice = drv->device;
  if (hDevice == INVALID_HANDLE_VALUE) {
    aml_printf("hDevice == INVALID_HANDLE_VALUE\n");
    return 0;
  }

  struct usbDevIoCtrl ctrl = {
    drv->handle, (char *) lpInBuffer, (char *) lpOutBuffer,
    nInBufferSize, nOutBufferSize, lpBytesReturned, lpOverlapped};
#ifndef BUGFIX
  struct usbDevIoCtrl unk = {
    NULL, NULL, NULL, 1, 0, NULL, (struct _OVERLAPPED *) hDevice};
#endif
  int doRead = 0;

  switch (dwIoControlCode) {
    case IOCTL_READ_MEM:
      return IOCTL_READ_MEM_Handler(ctrl);
    case IOCTL_WRITE_MEM:
      return IOCTL_WRITE_MEM_Handler(ctrl);
    case IOCTL_FILL_MEM:
      return IOCTL_FILL_MEM_Handler(ctrl);
    case IOCTL_RUN_IN_ADDR:
      return IOCTL_RUN_IN_ADDR_Handler(ctrl);
    case IOCTL_READ_LARGE_MEM:
      doRead = 1;
      [[fallthrough]];
    case IOCTL_WRITE_LARGE_MEM:
      return IOCTL_DO_LARGE_MEM_Handler(ctrl, doRead);
    case IOCTL_READ_AUX_REG:
      return IOCTL_READ_AUX_REG_Handler(ctrl);
    case IOCTL_WRITE_AUX_REG:
      return IOCTL_WRITE_AUX_REG_Handler(ctrl);
    case IOCTL_MODIFY_MEM:
      return IOCTL_MODIFY_MEM_Handler(ctrl);
    case IOCTL_IDENTIFY_HOST:
      return IOCTL_IDENTIFY_HOST_Handler(ctrl);
    case IOCTL_TPL_CMD:
      return IOCTL_TPL_CMD_Handler(ctrl);
    case IOCTL_TPL_STATUS:
      return IOCTL_TPL_STATUS_Handler(ctrl);
    default:
      return 0;
  }
}


int usbDeviceIoControlEx (
    struct AmlUsbDrv *drv, unsigned int dwIoControlCode, void *lpInBuffer,
    unsigned int nInBufferSize, void *lpOutBuffer, unsigned int nOutBufferSize,
    unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped,
    unsigned int timeout) {
  struct usb_device *hDevice = drv->device;
  if (hDevice == INVALID_HANDLE_VALUE) {
    aml_printf("hDevice == INVALID_HANDLE_VALUE\n");
    return 0;
  }

  struct usbDevIoCtrl ctrl = {
    drv->handle, (char *) lpInBuffer, (char *) lpOutBuffer,
    nInBufferSize, nOutBufferSize, lpBytesReturned, lpOverlapped};
#ifndef BUGFIX
  struct usbDevIoCtrl unk = {
    NULL, NULL, NULL, 1, 0, NULL, (struct _OVERLAPPED *) hDevice};
#endif

  switch (dwIoControlCode) {
    case IOCTL_WRITE_MEM:
      return IOCTL_WRITE_MEM_Handler(ctrl);
    case IOCTL_TPL_STATUS:
      return IOCTL_TPL_STATUS_Handler_Ex(ctrl, timeout);
    case IOCTL_WRITE_MEDIA:
      return IOCTL_WRITE_MEDIA_Handler(ctrl, timeout);
    case IOCTL_READ_MEDIA:
      return IOCTL_READ_MEDIA_Handler(ctrl, timeout);
    case IOCTL_BULK_CMD_1:
      return IOCTL_BULK_CMD_Handler(ctrl, timeout, 0x34);
    case IOCTL_BULK_CMD_2:
      return IOCTL_BULK_CMD_Handler(ctrl, timeout, 0x35);
    default:
      aml_printf("unknown control code 0x%x\n", dwIoControlCode);
      return 0;
  }
}


int usbReadFile (
    struct AmlUsbDrv *drv, void *lpOutBuffer, unsigned int nOutBufferSize,
    unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped) {
  (void) lpOverlapped;

  struct usb_device *hDevice = drv->device;
  if (hDevice == INVALID_HANDLE_VALUE) {
    return 0;
  }

  int ret = usb_bulk_read(
    drv->handle, drv->read_ep, (char *) lpOutBuffer, nOutBufferSize, 30000);

  *lpBytesReturned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "usbReadFile len=%d,ret=%d error_msg=%s\n", nOutBufferSize, ret,
      usb_strerror());
    return 0;
  }
  return 1;
}


int usbWriteFile (
    struct AmlUsbDrv *drv, const void *lpInBuffer, unsigned int nInBufferSize,
    unsigned int *lpBytesReturned, struct _OVERLAPPED *lpOverlapped) {
  (void) lpOverlapped;

  struct usb_device *hDevice = drv->device;
  if (hDevice == INVALID_HANDLE_VALUE) {
    return 0;
  }

  int ret = usb_bulk_write(
    drv->handle, drv->write_ep, (char *) lpInBuffer, nInBufferSize, 50000);
  *lpBytesReturned = (unsigned int) aml_max(0, ret);
  if (ret < 0) {
    aml_printf(
      "usbWriteFile len=%d,ret=%d error_msg=%s\n", nInBufferSize, ret,
      usb_strerror());
    return 0;
  }
  return 1;
}


int OpenUsbDevice (struct AmlUsbDrv *drv) {
  if (drv == NULL || drv->device == NULL || drv->handle != NULL) {
    aml_printf(
#ifndef BUGFIX
      "%s L%d bug %#x %#x %#x\n",
#else
      "%s L%d bug %p %p %p\n",
#endif
      "OpenUsbDevice", 570, (void *) drv,
      (void *) (drv == NULL ? NULL : drv->device),
      (void *) (drv == NULL ? NULL : drv->handle));
    return 0;
  }

  drv->handle = usb_open(drv->device);
  drv->read_ep = AM_EP_IN;
  drv->write_ep = AM_EP_OUT;
  return 1;
}


int CloseUsbDevice (struct AmlUsbDrv *drv) {
  if (drv == NULL || drv->device == NULL) {
    aml_printf(
#ifndef BUGFIX
      "%s L%d bug %#x %#x %#x\n",
#else
      "%s L%d bug %p %p %p\n",
#endif
      "CloseUsbDevice", 590, (void *) drv,
      (void *) (drv == NULL ? NULL : drv->device),
      (void *) (drv == NULL ? NULL : drv->handle));
    return 0;
  }

  usb_close(drv->handle);
  drv->handle = NULL;
  return 1;
}


int ResetDev (struct AmlUsbDrv *drv) {
  struct usb_device *hDevice = drv->device;
  if (hDevice == INVALID_HANDLE_VALUE) {
    return 0;
  }

  aml_printf("Reset WorldCup Device\n");
  return usb_reset(drv->handle);
}


int Aml_Libusb_Ctrl_RdWr (
    void *dev, uint32_t addr, char *buf, int len, int doRead, int timeout) {
  struct AmlUsbDrv drv = {(struct usb_device *) dev, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Fail in open dev\n");
    return -645;
  }

  int processed = 0;
  while (processed < len) {
    int chunk = aml_min(len - processed, 64);
    usb_control_msg(
      drv.handle,
      doRead ? USB_ENDPOINT_IN | USB_TYPE_VENDOR : USB_TYPE_VENDOR,
      doRead ? AM_REQ_READ_MEM : AM_REQ_WRITE_MEM,
      addr >> 16, addr & 0xFFFF, buf, chunk, timeout);
    processed += chunk;
    buf += chunk;
    addr += chunk;
  }

  CloseUsbDevice(&drv);
  return 0;
}


int Aml_Libusb_Password (void *dev, char *buf, int size, int timeout) {
  struct AmlUsbDrv drv = {(struct usb_device *) dev, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Fail in open dev\n");
    return -696;
  }

  if (size > 64) {
    aml_printf(
      "f(%s)size(%d) too large, cannot support it.\n", "Aml_Libusb_Password",
      size);
    return -703;
  }

  int value = 0;
  for (int i = 0; i < size; i++) {
    value += buf[i];
  }

  int ret = usb_control_msg(
    drv.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_PASSWORD,
    value, 0, buf, size, timeout);

  CloseUsbDevice(&drv);
  return ret;
}


int Aml_Libusb_get_chipinfo (
    void *dev, char *buf, int size, int index, int timeout) {
  struct AmlUsbDrv drv = {(struct usb_device *) dev, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Fail in open dev\n");
    return -748;
  }

  if (size > 64) {
    aml_printf(
      "f(%s)size(%d) too large, cannot support it.\n",
      "Aml_Libusb_get_chipinfo", size);
    return -755;
  }

  int ret = usb_control_msg(
    drv.handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, AM_REQ_CHIP_INFO,
    0, index, buf, size, timeout);

  CloseUsbDevice(&drv);
  return ret;
}


struct AmlBl2Cmd_u {
  int offset;
  int dataSize;
  int seq;
  int doCksum;
};

typedef struct AmlBl2Cmd_u AmlBl2Cmd_Download;


static int _Aml_Libusb_bl2_csw (
    usb_dev_handle *handle, bool doSend, bool doOkay, char **out) {
  static char csw[16];

  memset(csw, 0, sizeof(csw));
  if (doSend) {
    memcpy(csw, doOkay ? "OKAY" : "FAIL", 4);
  }

  int ret = usb_bulk_read(
    handle, doSend ? AM_EP_OUT : AM_EP_IN, csw, sizeof(csw), 30000);

  if (ret != sizeof(csw)) {
    aml_printf("[LUSB]ERR(L%d):", 814);
    aml_printf("Fail in [%s] csw, ret=%d\n", doSend ? "Send" : "Get", ret);
    return -815;
  }

  if (doSend) {
    return 0;
  }

  *out = csw;
  if (memcmp(&csw, "OKAY", 4) != 0) {
    aml_printf("[LUSB]ERR(L%d):", 820);
    aml_printf("Get CSW, result[%s]\n", csw);
    return -821;
  }

  return 0;
}


int Aml_Libusb_bl2_test (void *dev, int timeout, int argc, const char **argv) {
  struct AmlUsbDrv drv = {(struct usb_device *) dev, NULL, AM_EP_IN, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Fail in open dev\n");
    return -855;
  }

  if (argc <= 0) {
    aml_printf("[LUSB]ERR(L%d):", 868);
    aml_printf("need fip image path\n");
    return -869;
  }

  FILE *stream = NULL;
  char *buf = NULL;
  int ret;

  do {
    stream = fopen(argv[0], "rb");
    if (stream == NULL) {
      aml_printf("[LUSB]ERR(L%d):", 873);
      aml_printf("Fail in open file(%s)\n", argv[0]);
      ret = -874;
      goto end;
    }

    ret = usb_control_msg(
      drv.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_START,
      0x0200, 0x0000, NULL, 0, timeout);
    if (ret < 0) {
      aml_printf("Fail in start cmd, ret=%d\n", ret);
      ret = -885;
      goto end;
    }

    char cbw[0x200] = {};
    ret = usb_bulk_read(drv.handle, AM_EP_IN, cbw, sizeof(cbw), 30000);
    if (ret != sizeof(cbw)) {
      aml_printf("get cbw failed, want %d, ret %d\n", (int) sizeof(cbw), ret);
      ret = -896;
      goto end;
    }

    int seq = GET_U32(&cbw[4]);
    int dataSize = GET_U32(&cbw[8]);
    unsigned int offset = GET_U32(&cbw[12]);
    aml_printf(
      "[%s]dataSize=%u, offset=%u, seq %d\n", cbw, dataSize, offset, seq);
    aml_printf("[LUSB]requestType=%d\n", cbw[17]);
    if (cbw[17]) {
      ret = -904;
      goto end;
    }

    ret = _Aml_Libusb_bl2_csw(drv.handle, true, true, NULL);
    if (ret) {
      aml_printf("[LUSB]ERR(L%d):", 910);
      aml_printf("FAil in send csw, ret %d\n", ret);
      ret = -911;
      goto end;
    }

    fseek(stream, offset, SEEK_SET);
    buf = new char[0x10000];

    unsigned int cksum = 0;
    char *pcsw;

    unsigned int dataOffset = 0;
    for (unsigned int i = 0;
         i < (unsigned int) (dataSize + 0x10000 - 1) >> 16; i++) {
      int bulkSize = aml_min(dataSize - dataOffset, 0x10000);
      ret = usb_control_msg(
        drv.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_CMD4,
        dataOffset >> 9, bulkSize - 1, NULL, 0, timeout);
      if (ret < 0) {
        aml_printf("[LUSB]ERR(L%d):", 925);
        aml_printf("Fail in cmd4, ret %d\n", ret);
        ret = -926;
        goto end;
      }

      if (fread(buf, bulkSize, 1, stream) != 1) {
        aml_printf("[LUSB]ERR(L%d):", 931);
        aml_printf(
          "Fail in read file at offset 0x%lx\n", dataOffset + (long) offset);
        ret = -932;
        goto end;
      }

      ret = usb_bulk_write(drv.handle, AM_EP_OUT, buf, bulkSize, 30000);
      if (ret != bulkSize) {
        aml_printf("[LUSB]ERR(L%d):", 937);
        aml_printf("Fail in down data, want %d, but %d\n", bulkSize, ret);
        ret = -938;
        goto end;
      }

      pcsw = NULL;
      ret = _Aml_Libusb_bl2_csw(drv.handle, false, false, &pcsw);
      if (ret) {
        aml_printf("[LUSB]ERR(L%d):", 945);
        aml_printf("Fail get csw transfer, ret %d\n", ret);
        ret = -946;
        goto end;
      }

      cksum += checksum_64K(buf, bulkSize);
      dataOffset += 0x10000;
    }

    pcsw = NULL;
    ret = usb_control_msg(
      drv.handle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_CMD4,
      offset >> 9, 0x1FF, NULL, 0, timeout);
    if (ret < 0) {
      aml_printf("[LUSB]ERR(L%d):", 958);
      aml_printf("Fail in cmd4, ret %d\n", ret);
      ret = -959;
      goto end;
    }

    memcpy(buf, "AMLS", 4);
    SET_U32(&buf[4], seq);
    SET_U32(&buf[8], cksum);
    SET_U32(&buf[12], 0);
    ret = usb_bulk_write(drv.handle, AM_EP_OUT, buf, 0x200, 30000);
    if (ret != 0x200) {
      aml_printf("[LUSB]ERR(L%d):", 968);
      aml_printf("Fail in csw, ret =%d\n", ret);
      ret = -969;
      goto end;
    }

    aml_printf("[LUSB]before wait sum\n");
    aml_printf(
      "[LUSB]check sum %s\n",
      _Aml_Libusb_bl2_csw(drv.handle, false, false, &pcsw) ? "FAIL" : "OKAY");
    ret = 0;
  } while (0);

end:
  if (stream != NULL) {
    fclose(stream);
  }
  if (buf != NULL) {
    delete[] buf;
  }
  CloseUsbDevice(&drv);
  return ret;
}


static int _Aml_Libusb_bl2_secure_download (
    usb_dev_handle *dev, FILE *stream, char *buf, AmlBl2Cmd_Download *param) {
  unsigned int cksum = 0;
  char *pcsw;
  int ret;

  unsigned int dataOffset = 0;
  unsigned int dataSize = param->dataSize;
  for (int i = 0; i < (param->dataSize + 0x10000 - 1) >> 16; i++) {
    int bulkSize = aml_min(dataSize, 0x10000);
    ret = usb_control_msg(
      dev, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_CMD4,
      dataOffset >> 9, bulkSize - 1, NULL, 0, 5000);
    if (ret < 0) {
      aml_printf("[LUSB]ERR(L%d):", 1004);
      aml_printf("Fail in cmd4, ret %d\n", ret);
      return -1005;
    }

    if (fread(buf, bulkSize, 1, stream) != 1) {
      aml_printf("[LUSB]ERR(L%d):", 1010);
      aml_printf(
        "Fail in read file at offset 0x%lx\n",
        dataOffset + (long) param->offset);
      return -1011;
    }

    ret = usb_bulk_write(dev, AM_EP_OUT, buf, bulkSize, 30000);
    if (ret != bulkSize) {
      aml_printf("[LUSB]ERR(L%d):", 1016);
      aml_printf("Fail in down data, want %d, but %d\n", bulkSize, ret);
      return -1017;
    }

    pcsw = NULL;
    ret = _Aml_Libusb_bl2_csw(dev, false, false, &pcsw);
    if (ret) {
      aml_printf("[LUSB]ERR(L%d):", 1024);
      aml_printf("Fail get csw transfer, ret %d\n", ret);
      return -1025;
    }

    if (param->doCksum) {
      cksum += checksum_64K(buf, bulkSize);
    }
    dataOffset += 0x10000;
    dataSize -= 0x10000;
  }

  ret = usb_control_msg(
    dev, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_CMD4,
    param->offset >> 9, 0x1FF, NULL, 0, 5000);
  if (ret < 0) {
    aml_printf("[LUSB]ERR(L%d):", 1035);
    aml_printf("Fail in cmd4, ret %d\n", ret);
    return -1036;
  }

  memcpy(buf, "AMLS", 4);
  SET_U32(&buf[4], param->seq);
  SET_U32(&buf[8], cksum);
  SET_U32(&buf[12], 0);
  ret = usb_bulk_write(dev, AM_EP_OUT, buf, 0x200, 30000);
  if (ret != 0x200) {
    aml_printf("[LUSB]ERR(L%d):", 1046);
    aml_printf("Fail in csw, ret =%d\n", ret);
    return -1047;
  }

  aml_printf("[LUSB]before wait sum\n");
  pcsw = NULL;
  aml_printf(
    "[LUSB]check sum %s\n",
    _Aml_Libusb_bl2_csw(dev, false, false, &pcsw) ? "FAIL" : "OKAY");
  return 0;
}


static int _Aml_Libusb_bl2_cmd_parser (
    usb_dev_handle *dev, int *cmd, struct AmlBl2Cmd_u *param) {
  int ret;

  ret = usb_control_msg(
    dev, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, AM_REQ_START,
    0x0200, 0x0000, NULL, 0, 3000);
  if (ret < 0) {
    aml_printf("[LUSB]ERR(L%d):", 1069);
    aml_printf("Fail in start cmd, ret=%d\n", ret);
    return -1070;
  }

  char cbw[512] = {};
  ret = usb_bulk_read(dev, AM_EP_IN, cbw, sizeof(cbw), 3000);
  if (ret != sizeof(cbw)) {
    aml_printf("get cbw failed, want %d, ret %d\n", (int) sizeof(cbw), ret);
    return -1079;
  }

  if (cbw[17] == 0) {
    *cmd = cbw[16] >= 0 ? 0xABCD : 0xABCE;
  } else if (cbw[17] == 1) {
    aml_printf("[LUSB]BL2 END, waiting TPL plug-in...\n");
    *cmd = 0xABCF;
  } else {
    aml_printf("[LUSB]ERR(L%d):", 1086);
    aml_printf("bl2 report err, code=%d\n", cbw[17]);
    *cmd = 0xABD0;
  }

  if (*cmd != 0xABCF) {
    ret = _Aml_Libusb_bl2_csw(dev, true, false, NULL);
  } else {
    if (*cmd == 0xABCD) {
      param->offset = GET_U32(&cbw[12]);
      param->dataSize = GET_U32(&cbw[8]);
      param->seq = GET_U32(&cbw[4]);
      param->doCksum = (cbw[16] & 1) == 0;
      aml_printf(
        "[LUSB][%s]dataSize=%u, offset=%u, seq %d\n", cbw,
        param->dataSize, param->offset, param->seq);
      aml_printf("[LUSB]requestType=%d\n", cbw[17]);
    }
    ret = _Aml_Libusb_bl2_csw(dev, true, true, NULL);
  }

  if (ret) {
    aml_printf("[LUSB]ERR(L%d):", 1117);
    aml_printf("FAil in send csw, ret %d\n", ret);
    return -1118;
  }
  return 0;
}


int Aml_Libusb_bl2_boot (void *dev, int timeout, int argc, const char **argv) {
  (void) timeout;

  struct AmlUsbDrv drv = {(struct usb_device *) dev, NULL, AM_EP_IN, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Fail in open dev\n");
    return -1151;
  }

  if (argc <= 0) {
    aml_printf("[LUSB]ERR(L%d):", 1156);
    aml_printf("need fip image path\n");
    return -1157;
  }

  FILE *stream = NULL;
  char *buf = NULL;
  int ret;

  do {
    stream = fopen(argv[0], "rb");
    if (stream == NULL) {
      aml_printf("[LUSB]ERR(L%d):", 1164);
      aml_printf("Fail in open file(%s)\n", argv[0]);
      ret = -1165;
      goto end;
    }

    buf = new char[0x20000];
    int cmd;
    while (1) {
      struct AmlBl2Cmd_u param = {};
      ret = _Aml_Libusb_bl2_cmd_parser(drv.handle, &cmd, &param);
      if (ret) {
        aml_printf("[LUSB]ERR(L%d):", 1172);
        aml_printf("fail in parse cmd, ret=%d\n", ret);
        ret = -1173;
        goto end;
      }

      if (cmd <= 0xABCC || cmd > 0xABD0) {
        aml_printf("[LUSB]ERR(L%d):", 1176);
        aml_printf("Invalid cmd\n");
        ret = -1177;
        goto end;
      }
      if (cmd > 0xABCE) {
        break;
      }
      if (cmd != 0xABCD) {
        aml_printf("[LUSB]ERR(L%d):", 1192);
        aml_printf("cmd %X unsupported yet\n", cmd);
        goto end;
      }

      fseek(stream, param.offset, SEEK_SET);
      ret = _Aml_Libusb_bl2_secure_download(drv.handle, stream, buf, &param);
      if (ret) {
        aml_printf("[LUSB]ERR(L%d):", 1187);
        aml_printf("fail in download, sequence %d\n", param.seq);
        ret = -1188;
        goto end;
      }
    }
    ret = 0;
  } while (0);

end:
  if (stream != NULL) {
    fclose(stream);
  }
  if (buf != NULL) {
    delete[] buf;
  }
  CloseUsbDevice(&drv);
  return ret;
}
