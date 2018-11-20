#ifndef AML_USB_SCAN_X3_H
#define AML_USB_SCAN_X3_H

#include <usb.h>


#define AML_ID_VENDOR 0x1B8E
#define AML_ID_PRODUCE 0xC003


struct AmlscanX {
  const char *vendorName;
  struct usb_device **resultDevice;
  char *targetDevice;
  char **candidateDevices;
  int *nDevices;
};


int scanDevices (AmlscanX scan, const char *target);
int AmlScanUsbX3Devices (const char *vendorName, char **candidateDevices);
struct usb_device *AmlGetDeviceHandle (const char *vendorName, char *targetDevice);
int AmlGetMsNumber (char *a1, int a2, char *a3);
int AmlGetNeedDriver (unsigned short idVendor, unsigned short idProduct);
int AmlDisableSuspendForUsb (void);

#endif  // AML_USB_SCAN_X3_H
