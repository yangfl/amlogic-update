#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Amldbglog.h"
#include "AmlUsbScanX3.h"
#include "UsbRomDrv.h"
#include "scan.h"


static char *DeviceName[8];


int aml_scan_init (void) {
  printf("aml_scan_usbdev");
  for (int i = 0; i <= 7; ++i) {
    DeviceName[i] = (char *) malloc(0x100);
  }
  return 0;
}


int aml_scan_close (void) {
  for (int i = 0; i <= 7; ++i) {
    free(DeviceName[i]);
    DeviceName[i] = NULL;
  }
  return 0;
}


int aml_scan_usbdev (char **candidateDevices) {
  for (int i = 0; i <= 7; ++i) {
    memset(DeviceName[i], 0, 0x100);
  }

  int ret = AmlScanUsbX3Devices("WorldCup Device", DeviceName);
  printf("--%i \n", ret);
  if (ret <= 0) {
    return 0;
  }

  for (int i = 0; i < ret; ++i) {
    candidateDevices[i] = DeviceName[i];
  }
  return ret;
}


int aml_send_command (void *dev, char *cmd, int retry, char *reply) {
  char buf[128] = {};
  memcpy(buf, cmd, strlen(cmd));
  buf[66] = 1;

  unsigned int dataSize;
  struct AmlUsbRomRW rom = {(struct usb_device *) dev, 68, buf, &dataSize, 0};
  if (AmlUsbTplCmd(&rom) == 0) {
    struct AmlUsbRomRW rom = {(struct usb_device *) dev, 64, reply, NULL, 0};
    for (; retry > 0; retry--) {
      if (AmlUsbReadStatus(&rom) == 0) {
        printf("reply %s \n", reply);
        return 0;
      }
      usleep(100000);
    }
  }

  return -1;
}


int aml_get_sn (char *targetDevice, char *usid) {
  struct usb_device *dev = AmlGetDeviceHandle("WorldCup Device", targetDevice);
  if (dev == NULL) {
    return -1;
  }

  char hostID[4] = {};
  struct AmlUsbRomRW rom = {dev, sizeof(hostID), hostID, NULL, 0};
#ifdef BUGFIX
  unsigned int actualLen;
  rom.pDataSize = &actualLen;
#endif
  if (AmlUsbIdentifyHost(&rom) != 0) {
    dev = NULL;
    return -1;
  }
  if (hostID[3] != 16) {
    dev = NULL;
    return -1;
  }

  aml_send_command(dev, (char *) "efuse write version", 50, usid);
  int ret = aml_send_command(dev, (char *) "efuse read usid", 50, usid);
  printf("%s \n", usid);
  if (ret < 0) {
    dev = NULL;
    return -1;
  }

  char tmp1[256];
  char tmp2[256];

  memset(tmp1, 0, 8);
  memset(tmp2, 0, 8);

  ret = sscanf(usid, "%[^:]:(%[^)])", tmp1, tmp2);
  if (strcmp(tmp1, "success") != 0 || ret != 2) {
    return 0;
  }

  memset(usid, 0, 8);
  memcpy(usid, tmp2, strlen(tmp2));
  dev = NULL;
  return strlen(tmp2);
}


int aml_set_sn (char *targetDevice, char *usid) {
  struct usb_device *dev = AmlGetDeviceHandle("WorldCup Device", targetDevice);
  if (dev == NULL) {
    return -1;
  }

  char hostID[4] = {};
  struct AmlUsbRomRW rom = {dev, sizeof(hostID), hostID, NULL, 0};
  if (AmlUsbIdentifyHost(&rom) != 0) {
    dev = NULL;
    return -1;
  }
  if (hostID[3] != 16) {
    dev = NULL;
    return -1;
  }

  char cmd[256];
  sprintf(cmd, "efuse write usid %s", usid);

  char src[256];
  aml_send_command(dev, (char *) "efuse write version", 50, src);
  int ret = aml_send_command(dev, cmd, 50, src);
  if (ret < 0) {
    dev = NULL;
    return -1;
  }

  char tmp1[256] = {};
  char tmp2[256] = {};

  sscanf(src, "%[^:]:(%[^)])", tmp1, tmp2);
  printf("tmp1=%s,tmp2=%s \n", tmp1, tmp2);
  if (strcmp(tmp1, "success") != 0) {
    return 0;
  }

  dev = NULL;
  return strlen(tmp2);
}
