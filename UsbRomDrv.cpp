#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "checksum.h"
#include "defs.h"
#include "gettime.h"

#include "Amldbglog.h"
#include "AmlLibusb.h"
#include "UsbRomDrv.h"


static int ValidParamDWORD (unsigned int *arg) {
  (void) arg;

  return 1;
}


static int ValidParamVOID (void *arg) {
  (void) arg;

  return 1;
}


static int ValidParamHANDLE (void **arg) {
  (void) arg;

  return 1;
}


static unsigned char *itoa1 (int value, unsigned char *str) {
  SET_U32(str, value);
  return &str[4];
}


static int RWLargeMemCMD (
    struct AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
    int cksum, int seqNum, char doRead) {
  unsigned char buf[128] = {};
  itoa1(address, buf);
  itoa1(size, &buf[4]);
  itoa1(cksum, &buf[8]);
  itoa1(seqNum, &buf[12]);
  itoa1(bulkSize, &buf[64]);

  unsigned int bytesReturned = 0;
  return usbDeviceIoControl(
    drv, doRead ? IOCTL_READ_LARGE_MEM : IOCTL_WRITE_LARGE_MEM,
    buf, sizeof(buf), NULL, 0, &bytesReturned, NULL);
}


static int RWMediaCMD (
    struct AmlUsbDrv *drv, int address, int size, int seqNum, int cksum,
    int bulkSize, char doRead, unsigned int timeout) {
  unsigned char buf[128] = {};
  itoa1(address, buf);
  itoa1(size, &buf[4]);
  itoa1(cksum, &buf[8]);
  itoa1(seqNum, &buf[12]);
  itoa1(bulkSize, &buf[64]);

  unsigned int bytesReturned = 0;
  return usbDeviceIoControlEx(
    drv, doRead ? IOCTL_READ_MEDIA : IOCTL_WRITE_MEDIA,
    buf, sizeof(buf), NULL, 0, &bytesReturned, NULL, timeout);
}


static int WriteLargeMemCMD (
    struct AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
    int cksum, int writeSeqNum) {
  return RWLargeMemCMD(drv, address, size, bulkSize, cksum, writeSeqNum, 0);
}


static int ReadLargeMemCMD (
    struct AmlUsbDrv *drv, unsigned long address, int size, int bulkSize,
    int cksum, int readSeqNum) {
  return RWLargeMemCMD(drv, address, size, bulkSize, cksum, readSeqNum, 1);
}


static int WriteMediaCMD (
    struct AmlUsbDrv *drv, int address, int size, int seqNum, int cksum,
    int bulkSize, unsigned int timeout) {
  return RWMediaCMD(drv, address, size, seqNum, cksum, bulkSize, 0, timeout);
}


static int ReadMediaCMD (
    struct AmlUsbDrv *drv, int address, int size, int seqNum, int cksum,
    int bulkSize, unsigned int timeout) {
  return RWMediaCMD(drv, address, size, seqNum, cksum, bulkSize, 1, timeout);
}


static int write_bulk_usb (struct AmlUsbDrv *drv, char *buf, unsigned int len) {
  unsigned int ret = 0;
  if (usbWriteFile(drv, buf, len, &ret, NULL) == 0) {
    return -1;
  }
  return ret;
}


static int read_bulk_usb (struct AmlUsbDrv *drv, char *buf, unsigned int len) {
  unsigned int ret = 0;
  if (usbReadFile(drv, buf, len, &ret, NULL) == 0) {
    return -1;
  }
  return ret;
}


#ifndef BUGFIX
static int fill_mem_usb (struct AmlUsbDrv *drv, char *buf) {
  (void) drv;
  (void) buf;

  return -1;
}
#endif


static int rw_ctrl_usb (
    struct AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len,
    int doRead) {
  unsigned int bytesReturned = 0;
  if (doRead) {
    return usbDeviceIoControl(
      drv, IOCTL_READ_MEM, &ctrl, 4, buf, len, &bytesReturned, NULL);
  }

  unsigned char in_buf[64];
  itoa1(ctrl, in_buf);
  memcpy(&in_buf[4], buf, len);
  return usbDeviceIoControl(
    drv, IOCTL_WRITE_MEM, in_buf, len + 4, NULL, 0, &bytesReturned, NULL);
}


static int write_control_usb (
    struct AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len) {
  return rw_ctrl_usb(drv, ctrl, buf, len, 0);
}


static int read_control_usb (
    struct AmlUsbDrv *drv, unsigned long ctrl, char *buf, unsigned long len) {
  return rw_ctrl_usb(drv, ctrl, buf, len, 1);
}


#ifndef BUGFIX
static int read_usb_status (void *addr, char *buf, size_t len) {
  (void) addr;
  (void) buf;
  (void) len;

  return -1;
}
#endif


int AmlUsbWriteLargeMem (struct AmlUsbRomRW *rom) {
  static int WriteSeqNum = 0;

  if (ValidParamDWORD(&rom->bufferLen) == 0) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) == 0) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) == 0) {
    return -3;
  }

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -4;
  }

  unsigned int offset = 0;
  int sizeToSend = -1;
  int retry = 0;
  unsigned short cksum = checksum(
    (unsigned short *) rom->buffer, rom->bufferLen);
  for (int i = 0; i < 4; i++) {
    unsigned int remain = rom->bufferLen;
    while (remain > 0) {
      unsigned int transferSize = aml_min(remain, 0x1000u);
      if (sizeToSend == -1) {
        unsigned int totalSize = aml_min(remain, 0x10000u);
        int ret = WriteLargeMemCMD(
          &drv, rom->address, totalSize, transferSize, cksum, WriteSeqNum);
        if (ret == 0) {
          break;
        }
        sizeToSend = totalSize;
      }
      WriteSeqNum++;

      int actualLen = write_bulk_usb(&drv, &rom->buffer[offset], transferSize);
      if (actualLen != 0) {
        retry = 0;
      } else {
        retry++;
        if (retry > 5) {
          goto end;
        }
      }
      if (actualLen == -1) {
        goto end;
      }

      offset += actualLen;
      remain -= actualLen;
    }
    if (remain == 0) {
      break;
    }
  }

end:
  CloseUsbDevice(&drv);
  *rom->pDataSize = offset;
  return rom->bufferLen == offset ? 0 : -6;
}


int AmlUsbReadLargeMem (struct AmlUsbRomRW *rom) {
  if (ValidParamDWORD(&rom->bufferLen) == 0) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) == 0) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) == 0) {
    return -3;
  }

  static int ReadSeqNum = 0;
  ReadSeqNum++;

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -4;
  }

  unsigned short cksum = checksum(
    (unsigned short *) rom->buffer, rom->bufferLen);

  unsigned int offset = 0;
  int sizeToRead = -1;
  int retry = 0;
  for (int i = 0; i < 4; i++) {
    unsigned int remain = rom->bufferLen;
    while (remain > 0) {
      unsigned int transferSize = aml_min(remain, 0x10000u);
      if (sizeToRead == -1) {
        int ret = ReadLargeMemCMD(
          &drv, rom->address, rom->bufferLen,
          transferSize >= 0x1000 ? 0x1000 : aml_min(transferSize, 0x200u),
          cksum, ReadSeqNum);
        if (ret == 0) {
          break;
        }
        sizeToRead = transferSize;
      }

      int actualLen = read_bulk_usb(&drv, &rom->buffer[offset], transferSize);
      if (actualLen != 0) {
        retry = 0;
      } else {
        retry++;
        if (retry > 5) {
          goto end;
        }
      }
      if (actualLen == -1) {
        goto end;
      }

      offset += actualLen;
      remain -= actualLen;
    }
    if (remain == 0) {
      break;
    }
  }

end:
  CloseUsbDevice(&drv);
  *rom->pDataSize = offset;
  return rom->bufferLen == offset ? 0 : -6;
}


int AmlUsbReadMemCtr (struct AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return 0;
  }

  unsigned int remain = rom->bufferLen;
  char *buf = rom->buffer;
  while (remain > 0) {
    unsigned int transferSize = aml_min(rom->bufferLen, 64u);
    if (read_control_usb(&drv, rom->address, buf, transferSize) == 0) {
      break;
    }

    rom->address += transferSize;
    buf += transferSize;
    remain -= transferSize;
  }

  CloseUsbDevice(&drv);
  *rom->pDataSize = buf - rom->buffer;
  return remain == 0 ? 0 : -1;
}


int AmlUsbWriteMemCtr (struct AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return 0;
  }

  unsigned int remain = rom->bufferLen;
  char *buf = rom->buffer;
  while (remain > 0) {
    unsigned int transferSize = aml_min(rom->bufferLen, 64u);
    if (write_control_usb(&drv, rom->address, buf, transferSize) == 0) {
      break;
    }

    rom->address += transferSize;
    buf += transferSize;
    remain -= transferSize;
  }

  CloseUsbDevice(&drv);
  return remain == 0;
}


int AmlUsbRunBinCode (struct AmlUsbRomRW *rom) {
  aml_printf("AmlUsbRunBinCode:ram_addr=%08x\n", rom->address);

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  unsigned int addr = htole32(rom->address);
  int ret = usbDeviceIoControl(
    &drv, IOCTL_RUN_IN_ADDR, &addr, sizeof(addr), NULL, 0, NULL, NULL);

  CloseUsbDevice(&drv);
  return ret != 0 ? 0 : -2;
}


int AmlUsbIdentifyHost (struct AmlUsbRomRW *rom) {
  aml_printf("AmlUsbIdentifyHost\n");

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControl(
    &drv, IOCTL_IDENTIFY_HOST, NULL, 0, rom->buffer, rom->bufferLen,
    rom->pDataSize, NULL);

  CloseUsbDevice(&drv);
  return ret != 0 ? 0 : -2;
}


int AmlUsbTplCmd (struct AmlUsbRomRW *rom) {
  aml_printf("AmlUsbTplCmd = %s ", rom->buffer);

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControl(
    &drv, IOCTL_TPL_CMD, rom->buffer, rom->bufferLen, NULL, 0, rom->pDataSize,
    NULL);

  aml_printf("rettemp = %d buffer = %s\n", ret, rom->buffer);
  CloseUsbDevice(&drv);
  return ret != 0 ? 0 : -2;
}


int AmlUsbburn (
    struct usb_device *dev, const char *filename, unsigned int address,
    char *memType, int nBytes, size_t bulkSize, int cksum) {
  if (dev == NULL) {
    return -15;
  }

  FILE *stream = fopen(filename, "rb");
  if (stream == NULL) {
    return -25;
  }

  struct AmlUsbRomRW rom = {dev, 0, NULL, NULL, address};
  char *buf = (char *) malloc(bulkSize);

  fseek(stream, 0, SEEK_END);
  size_t fileSize = ftell(stream);
  fseek(stream, 0, SEEK_SET);

  unsigned int offset = 0;
  int ret;
  while (fileSize != 0) {
    size_t transferSize = aml_min(fileSize, bulkSize);
    fread(buf, 1, transferSize, stream);

    unsigned int dataSize;
    rom.buffer = buf;
    rom.bufferLen = transferSize;
    rom.pDataSize = &dataSize;
    ret = AmlUsbBurnWrite(&rom, memType, nBytes, cksum);
    if (ret) {
      break;
    }

    fileSize -= dataSize;
    rom.address += dataSize;
    offset += dataSize;
    fseek(stream, offset, SEEK_SET);
  }

  if (buf != NULL) {
    free(buf);
  }
  if (stream != NULL) {
    fclose(stream);
  }
  return ret ? ret : offset;
}


int AmlUsbReadStatus (struct AmlUsbRomRW *rom) {
  aml_printf("AmlUsbReadStatus ");

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControl(
    &drv, IOCTL_TPL_STATUS, rom->buffer, 4, rom->buffer, rom->bufferLen,
    rom->pDataSize, NULL);
  aml_printf("retusb = %d\n", ret);

  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlUsbReadStatusEx (struct AmlUsbRomRW *rom, unsigned int timeout) {
  aml_printf("AmlUsbReadStatus ");

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControlEx(
    &drv, IOCTL_TPL_STATUS, rom->buffer, 4, rom->buffer, rom->bufferLen,
    rom->pDataSize, NULL, timeout);
  aml_printf("retusb = %d\n", ret);

  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlResetDev (struct AmlUsbRomRW *rom) {
  if (rom->device == NULL) {
    return -1;
  }

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -2;
  }

  aml_printf("reset worldcup device\n");
  int ret = ResetDev(&drv);

  CloseUsbDevice(&drv);
  return ret;
}


int AmlSetFileCopyComplete (struct AmlUsbRomRW *rom, char *buf) {
  (void) rom;
  (void) buf;

  aml_printf("%s L%d not implemented", "AmlSetFileCopyComplete", 598);
  return 0;
}


int AmlGetUpdateComplete (struct AmlUsbRomRW *rom, char *buf, void *a3) {
  (void) rom;
  (void) buf;
  (void) a3;

  aml_printf("%s L%d not implemented", "AmlGetUpdateComplete", 664);
  return 0;
}


int AmlSetFileCopyCompleteEx (
    struct AmlUsbRomRW *rom, char *buf, unsigned int len) {
  (void) rom;
  (void) buf;
  (void) len;

  aml_printf("%s L%d not implemented", "AmlSetFileCopyCompleteEx", 732);
  return 0;
}


int AmlWriteMedia (struct AmlUsbRomRW *rom) {
  if (ValidParamDWORD(&rom->bufferLen) == 0) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) == 0) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) == 0) {
    return -3;
  }

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Open device failed\n");
    return -4;
  }

  unsigned int cksum = checksum_64K(rom->buffer, rom->bufferLen);

  int ret = 0;
  for (int addr = 0; addr <= 2; addr++) {
    unsigned int bulkSize = aml_min(rom->bufferLen, 0x10000u);
    unsigned int cmd = 16 * rom->address;
    ret = WriteMediaCMD(&drv, addr, rom->bufferLen, cksum, cmd, bulkSize, 5000);
    if (ret == 0) {
      aml_printf("Write media command %d failed\n", cmd);
      ret = -6;
      break;
    }

    unsigned int actualLen = 0;
    ret = usbWriteFile(&drv, rom->buffer, bulkSize, &actualLen, NULL);
    if (ret == 0) {
      aml_printf("usbReadFile failed ret=%d", ret);
      ret = -7;
      break;
    }
    if (bulkSize != actualLen) {
      aml_printf("[AmlUsbRom]Err:");
      aml_printf("Want Write 0x%x, but actual_len 0x%x\n", bulkSize, actualLen);
      ret = -799;
      break;
    }

    ret = 0;
    char buf[0x200] = {};
    time_t startTime = GetTickCount();
    for (time_t curTime = startTime; curTime - startTime < 12 * 60 * 1000;
         curTime = GetTickCount()) {
      if (usbReadFile(&drv, buf, sizeof(buf), &actualLen, NULL) == 0) {
        aml_printf("[AmlUsbRom]Err:");
        aml_printf("usbReadFile failed ret=%d", 0);
        ret = -811;
        break;
      }

      const char *strBusy = "Continue:32";
      unsigned int strLenBusy = strlen(strBusy);
      if (actualLen < strLenBusy) {
        aml_printf("[AmlUsbRom]Err:");
        aml_printf("return size %d < strLenBusy %d\n", actualLen, strLenBusy);
        ret = -816;
        break;
      }

      if (strncmp(buf, strBusy, strLenBusy) != 0) {
        break;
      }

      usleep(500000);
    }

    if (ret == 0) {
      ret = strncmp(buf, "OK!!", 4);
      if (ret != 0) {
        continue;
      }
    }
    break;
  }

  CloseUsbDevice(&drv);
  return ret;
}


int AmlReadMedia (struct AmlUsbRomRW *rom) {
#ifndef BUGFIX
  unsigned char buf[0x200] = {};
#endif

  if (ValidParamDWORD(&rom->bufferLen) == 0) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) == 0) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) == 0) {
    return -3;
  }

  struct AmlUsbDrv drv = {rom->device, NULL, 0, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("Open device failed\n");
    return -4;
  }

  if (ReadMediaCMD(&drv, 0, rom->bufferLen, 0, 0, 0, 5000) == 0) {
    aml_printf("Read media command failed\n");
    CloseUsbDevice(&drv);
    return -5;
  }

  memset(rom->buffer, 0, rom->bufferLen);
  unsigned int actualLen = 0;
  int ret = usbReadFile(&drv, rom->buffer, rom->bufferLen, &actualLen, NULL);
  if (ret == 0) {
    aml_printf("usbReadFile failed ret=%d\n", ret);
    CloseUsbDevice(&drv);
    return -6;
  }

  if (rom->bufferLen != actualLen) {
    aml_printf(
      "Want read %d bytes, actual len %d\n", rom->bufferLen, actualLen);
    CloseUsbDevice(&drv);
    return -7;
  }

  *rom->pDataSize = actualLen;
  CloseUsbDevice(&drv);
  return 0;
}


int AmlUsbBulkCmd (struct AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {rom->device, NULL, AM_EP_OUT, 0};
  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("[AmlUsbRom]Err:");
    aml_printf("FAil in OpenUsbDevice\n");
    return -926;
  }

  aml_printf("AmlUsbBulkCmd[%s]\n", rom->buffer);
  if (usbDeviceIoControlEx(
      &drv, IOCTL_BULK_CMD_1, rom->buffer, rom->bufferLen, NULL, 0,
      rom->pDataSize, NULL, 5000) == 0) {
    aml_printf("[AmlUsbRom]Err:");
    aml_printf("rettemp = %d buffer = [%s]\n", 0, rom->buffer);
    CloseUsbDevice(&drv);
    return -2;
  }

  char buf[0x200] = {};
  bool success = true;
  time_t startTime = GetTickCount();
  for (time_t curTime = startTime; curTime - startTime < 20 * 60 * 1000;
       curTime = GetTickCount()) {
    const char *strBusy = "Continue:34";
    int strLenBusy = strlen(strBusy);

    int ret = read_bulk_usb(&drv, buf, sizeof(buf));
    if (ret < strLenBusy) {
      aml_printf("[AmlUsbRom]Err:");
      aml_printf("return len=%d < strLenBusy %d\n", ret, strLenBusy);
      success = false;
      break;
    }
    if (strncmp(buf, strBusy, strLenBusy) != 0) {
      break;
    }

    aml_printf("[AmlUsbRom]Inf:");
    aml_printf("(bootloader)%s\n", buf);
    usleep(3000000);
  }

  if (success) {
    success = strncmp(buf, "success", 7) == 0;
  }

  aml_printf("[AmlUsbRom]Inf:");
  aml_printf("bulkInReply %s\n", buf);
  CloseUsbDevice(&drv);
  return success ? 0 : -2;
}


int AmlUsbCtrlWr (struct AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {rom->device, NULL, AM_EP_OUT, 0};
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControlEx(
    &drv, IOCTL_WRITE_MEM, rom->buffer, rom->bufferLen, NULL, 0, rom->pDataSize,
    NULL, 5000);

  CloseUsbDevice(&drv);
  return ret != 0 ? 0 : -2;
}


int AmlUsbBurnWrite (
    struct AmlUsbRomRW *cmd, char *memType, size_t nBytes, int cksum) {
  int ret = AmlUsbWriteLargeMem(cmd);
  if (ret) {
    return ret;
  }

  unsigned int oldDataSize = *cmd->pDataSize;

  char buf[88] = {};
  SET_U32(&buf[0], cmd->address);
  SET_U32(&buf[4], 0);
  SET_U32(&buf[8], cksum);
  SET_U32(&buf[24], cmd->bufferLen);
  SET_U32(&buf[28], cksum);
  SET_U64(&buf[32], nBytes);
  memcpy(&buf[48], memType, strlen(memType));
  SET_U32(&buf[80], 0);

  cmd->bufferLen = sizeof(buf);
  cmd->buffer = buf;
  ret = AmlUsbTplCmd(cmd);

  *cmd->pDataSize = ret != 0 ? 0 : oldDataSize;
  return ret;
}
