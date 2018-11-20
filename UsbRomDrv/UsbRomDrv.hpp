#ifndef AML_USB_ROM_DRV_H
#define AML_USB_ROM_DRV_H

#include <usb.h>

#include "../AmlLibusb/AmlLibusb.hpp"


int ValidParamDWORD (unsigned int *);
int ValidParamVOID (void *);
int ValidParamHANDLE (void **);
unsigned char *itoa1 (int value, unsigned char *str);
int RWLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                   int checksum, int seqNum, char readOrWrite);
int RWMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                int bulkSize, char readOrWrite, unsigned int timeout);
int WriteLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                      int checksum, int writeSeqNum);
int ReadLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                     int checksum, int readSeqNum);
int WriteMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                   int bulkSize, unsigned int timeout);
int ReadMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                  int bulkSize, unsigned int timeout);
int write_bulk_usb (AmlUsbDrv *drv, char *buf, unsigned int len);
int read_bulk_usb (AmlUsbDrv *drv, char *buf, unsigned int len);
int fill_mem_usb (AmlUsbDrv *drv, char *buf);
int rw_ctrl_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len,
                 int readOrWrite);
int write_control_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len);
int read_control_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len);
int read_usb_status (void *addr, char *buf, size_t len);
unsigned short checksum_add (unsigned short *buf, int len, int noFlip);
unsigned int checksum_64K (void *buf, int len);
unsigned short originale_add (unsigned short *buf, int len);
unsigned short checksum (unsigned short *buf, int len);

#endif  // AML_USB_ROM_DRV_H
