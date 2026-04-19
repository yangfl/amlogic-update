#ifndef AML_USB_SCAN_X3_H
#define AML_USB_SCAN_X3_H

#include "usbcompat.h"


#define AML_ID_VENDOR 0x1B8E
#define AML_ID_PRODUCE 0xC003


int AmlScanUsbX3Devices (const char *vendorName, char **candidateDevices);
struct usb_device *AmlGetDeviceHandle (
  const char *vendorName, char *targetDevice);
int AmlGetMsNumber (char *targetDevice, int a2, char *vendorName);
int AmlGetNeedDriver (unsigned short idVendor, unsigned short idProduct);
int AmlDisableSuspendForUsb (void);


#endif  // AML_USB_SCAN_X3_H
