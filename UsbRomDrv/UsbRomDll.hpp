#ifndef AML_USB_ROM_DLL_H
#define AML_USB_ROM_DLL_H

#include <usb.h>

#include "../AmlLibusb/AmlLibusb.hpp"


struct AmlUsbRomRW {
  struct usb_device *device;
  unsigned int bufferLen;
  char *buffer;
  unsigned int *pDataSize;
  unsigned int address;
};


int AmlUsbWriteLargeMem (AmlUsbRomRW *rom);
int AmlUsbReadLargeMem (AmlUsbRomRW *rom);
int AmlUsbReadMemCtr (AmlUsbRomRW *rom);
int AmlUsbWriteMemCtr (AmlUsbRomRW *rom);
int AmlUsbRunBinCode (AmlUsbRomRW *rom);
int AmlUsbIdentifyHost (AmlUsbRomRW *rom);
int AmlUsbTplCmd (AmlUsbRomRW *rom);
int AmlUsbburn (struct usb_device *device, const char *filename, unsigned int address,
                const char *memType, int nBytes, size_t bulkTransferSize, int checksum);

int AmlUsbReadStatus (AmlUsbRomRW *rom);
int AmlUsbReadStatusEx (AmlUsbRomRW *rom, unsigned int timeout);
int AmlResetDev (AmlUsbRomRW *rom);
int AmlSetFileCopyComplete (AmlUsbRomRW *rom);
int AmlGetUpdateComplete (AmlUsbRomRW *rom);
int AmlSetFileCopyCompleteEx (AmlUsbRomRW *rom);
int AmlWriteMedia (AmlUsbRomRW *rom);
int AmlReadMedia (AmlUsbRomRW *rom);
int AmlUsbBulkCmd (AmlUsbRomRW *rom);
int AmlUsbCtrlWr (AmlUsbRomRW *rom);

int AmlUsbBurnWrite (AmlUsbRomRW *cmd, char *memType, unsigned long long nBytes,
                     int checksum);


#endif  // AML_USB_ROM_DLL_H
