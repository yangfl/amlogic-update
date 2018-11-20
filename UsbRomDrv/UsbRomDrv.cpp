#include <algorithm>
#include <cstring>
#include <cstdio>

#include "UsbRomDrv.hpp"
#include "../Amldbglog/Amldbglog.h"
#include "../AmlTime.h"
#include "../defs.h"


int ValidParamDWORD (unsigned int *a1) {
  unsigned int v1 = *a1;
  return 1;
}


int ValidParamVOID (void *a1) {
  char v1 = *(char *) a1;
  return 1;
}


int ValidParamHANDLE (void **a1) {
  void *v1 = *a1;
  return 1;
}


unsigned char *itoa1 (int value, unsigned char *str) {
  unsigned char *result; // rax

  for (int i = 0; i <= 3; ++i) {
    result = &str[i];
    *result = (unsigned char) (value >> 8 * i);
  }
  return result;
}


int RWLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                   int checksum, int seqNum, char readOrWrite) {
  unsigned int inDataLen = 0; // [rsp+38h] [rbp-98h]
  unsigned char buf[128] = {}; // [rsp+40h] [rbp-90h]

  itoa1(address, buf);
  itoa1(size, &buf[4]);
  itoa1(checksum, &buf[8]);
  itoa1(seqNum, &buf[12]);
  itoa1(bulkSize, &buf[64]);
  return usbDeviceIoControl(drv, readOrWrite ? 0x80002010 : 0x80002014, buf, sizeof(buf),
                            nullptr, 0, &inDataLen, nullptr);
}


int RWMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                int bulkSize, char readOrWrite, unsigned int timeout) {
  // esi
  unsigned int inDataLen = 0; // [rsp+38h] [rbp-98h]
  unsigned char buf[128] = {}; // [rsp+40h] [rbp-90h]

  itoa1(address, buf);
  itoa1(size, &buf[4]);
  itoa1(checksum, &buf[8]);
  itoa1(seqNum, &buf[12]);
  itoa1(bulkSize, &buf[64]);
  return usbDeviceIoControlEx(drv, readOrWrite ? 0x8000204C : 0x80002048, buf,
                              sizeof(buf), nullptr, 0, &inDataLen, nullptr, timeout);
}


int WriteLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                      int checksum, int writeSeqNum) {
  return RWLargeMemCMD(drv, address, size, bulkSize, checksum, writeSeqNum, 0);
}


int ReadLargeMemCMD (AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
                     int checksum, int readSeqNum) {
  return RWLargeMemCMD(drv, address, size, bulkSize, checksum, readSeqNum, 1);
}


int WriteMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                   int bulkSize, unsigned int timeout) {
  return RWMediaCMD(drv, address, size, seqNum, checksum, bulkSize, 0, timeout);
}


int ReadMediaCMD (AmlUsbDrv *drv, int address, int size, int seqNum, int checksum,
                  int bulkSize, unsigned int timeout) {
  return RWMediaCMD(drv, address, size, seqNum, checksum, bulkSize, 1, timeout);
}


int write_bulk_usb (AmlUsbDrv *drv, char *buf, unsigned int len) {
  int result;

  result = 0;
  if (usbWriteFile(drv, buf, len, (unsigned int *) &result) != 1) {
    result = -1;
  }
  return result;
}


int read_bulk_usb (AmlUsbDrv *drv, char *buf, unsigned int len) {
  int result;

  result = 0;
  if (usbReadFile(drv, buf, len, (unsigned int *) &result) != 1) {
    result = -1;
  }
  return result;
}


int fill_mem_usb (AmlUsbDrv *drv, char *buf) {
  return -1;
}


int rw_ctrl_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len,
                 int readOrWrite) {
  unsigned int read = 0;
  if (readOrWrite) {
    return usbDeviceIoControl(drv, 0x80002000, &ctrl, 4, buf, len, &read, nullptr);
  } else {
    unsigned char in_buf[64];
    itoa1(ctrl, in_buf);
    memcpy(&in_buf[4], buf, len);
    return usbDeviceIoControl(drv, 0x80002004, in_buf, len + 4, nullptr, 0, &read,
                              nullptr);
  }
}


int write_control_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len) {
  return rw_ctrl_usb(drv, ctrl, buf, len, 0);
}


int read_control_usb (AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len) {
  return rw_ctrl_usb(drv, ctrl, buf, len, 1);
}


int read_usb_status (void *addr, char *buf, size_t len) {
  return -1;
}


unsigned short checksum_add (unsigned short *buf, int len, int noFlip) {
  unsigned int checksum = 0;

  for (; len > 1; len -= 2) {
    checksum += le16toh(*buf);
    ++buf;
  }
  if (len) {
    checksum += *(unsigned char *) buf;
  }
  checksum = (checksum >> 16) + (unsigned short) checksum;
  checksum = (checksum >> 16) + (unsigned short) checksum;
  if (!noFlip) {
    checksum = ~checksum;
  }
  return (unsigned short) checksum;
}


unsigned int checksum_64K (void *buf, int len) {
  unsigned int checksum = 0;

  // process an int every time
  for (int div = len >> 2; div >= 0; div--) {
    checksum += le32toh(*(unsigned int *) buf);
    buf = (char *) buf + 4;
  }
  switch (len & 3) {
    case 1:
      checksum += *(unsigned char *) buf;
      break;
    case 2:
      checksum += le16toh(*(unsigned short *) buf);
      break;
    case 3:
      checksum += le32toh(*(unsigned int *) buf) & 0xFFFFFF;
      break;
    default:
      break;
  }
  return checksum;
}


unsigned short originale_add (unsigned short *buf, int len) {
  return checksum_add(buf, len, 1);
}


unsigned short checksum (unsigned short *buf, int len) {
  return checksum_add(buf, len, 0);
}
