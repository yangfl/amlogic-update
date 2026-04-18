#ifndef USB_ROM_DRV_H
#define USB_ROM_DRV_H

#include <stddef.h>

struct usb_device;


struct AmlUsbRomRW {
  struct usb_device *device;
  unsigned int bufferLen;
  char *buffer;
  unsigned int *pDataSize;
  unsigned int address;
};


#ifdef __cplusplus
extern "C" {
#endif

int AmlUsbWriteLargeMem (struct AmlUsbRomRW *rom);
int AmlUsbReadLargeMem (struct AmlUsbRomRW *rom);
int AmlUsbReadMemCtr (struct AmlUsbRomRW *rom);
int AmlUsbWriteMemCtr (struct AmlUsbRomRW *rom);
int AmlUsbRunBinCode (struct AmlUsbRomRW *rom);
int AmlUsbIdentifyHost (struct AmlUsbRomRW *rom);
int AmlUsbTplCmd (struct AmlUsbRomRW *rom);
int AmlUsbburn (
  struct usb_device *dev, const char *filename, unsigned int address,
  char *memType, int nBytes, size_t bulkSize, int cksum);

int AmlUsbReadStatus (struct AmlUsbRomRW *rom);
int AmlUsbReadStatusEx (struct AmlUsbRomRW *rom, unsigned int timeout);
int AmlResetDev (struct AmlUsbRomRW *rom);
int AmlSetFileCopyComplete (struct AmlUsbRomRW *rom, char *buf);
int AmlGetUpdateComplete (struct AmlUsbRomRW *rom, char *buf, void *a3);
int AmlSetFileCopyCompleteEx (
  struct AmlUsbRomRW *rom, char *buf, unsigned int len);
int AmlWriteMedia (struct AmlUsbRomRW *rom);
int AmlReadMedia (struct AmlUsbRomRW *rom);
int AmlUsbBulkCmd (struct AmlUsbRomRW *rom);
int AmlUsbCtrlWr (struct AmlUsbRomRW *rom);

#ifdef __cplusplus
}
#endif

int AmlUsbBurnWrite (
  struct AmlUsbRomRW *cmd, char *memType, size_t nBytes, int cksum);


#endif  // USB_ROM_DRV_H
