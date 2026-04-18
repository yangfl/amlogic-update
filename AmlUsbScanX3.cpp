#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <usb.h>

#include "Amldbglog.h"
#include "AmlUsbScanX3.h"


struct AmlscanX {
  const char *vendorName;
  struct usb_device **resultDevice;
  char *targetDevice;
  char **candidateDevices;
  int *nDevices;
};


static int gLevel;
static char gHasSetDebugLevel;


static int scanDevices (struct AmlscanX scan, const char *target) {
  if (strcmp(scan.vendorName, "WorldCup Device") != 0) {
    aml_printf("[Scan][ERR]L%03d:", 159);
    aml_printf("Only supports scanning for [%s]\n", "WorldCup Device");
    return 0;
  }

  usb_init();
  usb_find_busses();
  usb_find_devices();

  int i = 0;
  for (struct usb_bus *bus = usb_busses; bus != NULL; bus = bus->next) {
    for (struct usb_device *dev = bus->devices; dev != NULL; dev = dev->next) {
      if (dev->descriptor.idVendor != AML_ID_VENDOR ||
          dev->descriptor.idProduct != AML_ID_PRODUCE) {
        continue;
      }

      char buf[128];
      snprintf(
        buf, sizeof(buf), "Bus %03d Device %03d: ID %04x:%04x",
        dev->bus ? dev->bus->location : 0, dev->devnum,
        AML_ID_VENDOR, AML_ID_PRODUCE);
      if (target == NULL) {
        strcpy(scan.candidateDevices[i], buf);
      } else if (strcmp(buf, target) == 0) {
        *scan.resultDevice = dev;
        return 1;
      }

      i++;
    }
  }

  *scan.nDevices = i;
  return 1;
}


int AmlScanUsbX3Devices (const char *vendorName, char **candidateDevices) {
  int nDevices = 0;
  struct AmlscanX scan = {vendorName, NULL, NULL, candidateDevices, &nDevices};

  gLevel = 0;
  if (gHasSetDebugLevel != 1) {
    gHasSetDebugLevel = 1;
  }
  if (scanDevices(scan, NULL) == 0) {
    aml_printf("AmlScanUsbX3Devices:scan usb devices failed\n");
    nDevices = -1;
  }
  return nDevices;
}


struct usb_device *AmlGetDeviceHandle (
    const char *vendorName, char *targetDevice) {
  int nDevices = 0;
  struct usb_device *resultDevice = NULL;
  char *candidateDevices[8] = {};
  struct AmlscanX scan = {
    vendorName, &resultDevice, targetDevice, candidateDevices, &nDevices};

  gLevel = 0;
  for (int i = 0; i <= 7; ++i) {
    candidateDevices[i] = (char *) malloc(0x100);
    memset(candidateDevices[i], 0, 0x100);
  }
  if (scanDevices(scan, targetDevice) == 0) {
    aml_printf("get usb devices handle failed\n");
    resultDevice = NULL;
  }
  for (int i = 0; i <= 7; ++i) {
    if (candidateDevices[i]) {
      free(candidateDevices[i]);
    }
  }
  return resultDevice;
}


int AmlGetMsNumber (char *targetDevice, int a2, char *vendorName) {
  (void) targetDevice;
  (void) a2;
  (void) vendorName;

  aml_printf("%s L%d not implemented", "AmlGetMsNumber", 292);
  return 0;
}


int AmlGetNeedDriver (unsigned short idVendor, unsigned short idProduct) {
  (void) idVendor;
  (void) idProduct;

  aml_printf("%s L%d not implemented", "AmlGetNeedDriver", 301);
  return 0;
}


int AmlDisableSuspendForUsb (void) {
  aml_printf("%s L%d not implemented", "AmlDisableSuspendForUsb", 311);
  return 0;
}
