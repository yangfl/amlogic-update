#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "AmlUsbScanX3.h"
#include "../Amldbglog/Amldbglog.h"


int gLevel;
char gHasSetDebugLevel;


int scanDevices (AmlscanX scan, const char *target) {
  if (strcmp(scan.vendorName, "WorldCup Device") != 0) {
    aml_printf("[Scan][ERR]L%03d:", 159);
    aml_printf("Only supports scanning for [%s]\n", "WorldCup Device");
    return 0;
  }

  usb_init();
  usb_find_busses();
  usb_find_devices();

  int iDevice = 0;
  for (struct usb_bus *usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) {
    for (struct usb_device *usb_device = usb_bus->devices; usb_device;
         usb_device = usb_device->next) {
      if (usb_device->descriptor.idVendor == AML_ID_VENDOR &&
          usb_device->descriptor.idProduct == AML_ID_PRODUCE) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Bus %03d Device %03d: ID %04x:%04x",
                 usb_device->bus ? usb_device->bus->location : 0, usb_device->devnum,
                 AML_ID_VENDOR, AML_ID_PRODUCE);
        if (target != NULL && !strcmp(tmp, target)) {
          *scan.resultDevice = usb_device;
          return 1;
        }
        strcpy(scan.candidateDevices[iDevice], tmp);
        ++iDevice;
      }
    }
  }
  *scan.nDevices = iDevice;
  return 1;
}


int AmlScanUsbX3Devices (const char *vendorName, char **candidateDevices) {
  int nDevices = 0;
  struct AmlscanX scan = {.vendorName = vendorName, .candidateDevices = candidateDevices, .nDevices = &nDevices};

  gLevel = 0;
  if (gHasSetDebugLevel != 1) {
    gHasSetDebugLevel = 1;
  }
  if (!scanDevices(scan, NULL)) {
    aml_printf("AmlScanUsbX3Devices:scan usb devices failed\n");
    nDevices = -1;
  }
  return nDevices;
}


struct usb_device *AmlGetDeviceHandle (const char *vendorName, char *targetDevice) {
  int nDevices = 0;
  struct usb_device *resultDevice = NULL;
  char *candidateDevices[8] = {};
  struct AmlscanX scan = {.vendorName = vendorName, .resultDevice = &resultDevice, .targetDevice = targetDevice, .candidateDevices = candidateDevices, .nDevices = &nDevices};

  gLevel = 0;
  for (int i = 0; i <= 7; ++i) {
    candidateDevices[i] = (char *) malloc(0x100);
    memset(candidateDevices[i], 0, 0x100);
  }
  if (!scanDevices(scan, targetDevice)) {
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


int AmlGetMsNumber (char *a1, int a2, char *a3) {
  aml_printf("%s L%d not implemented", "AmlGetMsNumber", 292);
  return 0;
}


int AmlGetNeedDriver (unsigned short idVendor, unsigned short idProduct) {
  aml_printf("%s L%d not implemented", "AmlGetNeedDriver", 301);
  return 0;
}


int AmlDisableSuspendForUsb (void) {
  aml_printf("%s L%d not implemented", "AmlDisableSuspendForUsb", 311);
  return 0;
}
