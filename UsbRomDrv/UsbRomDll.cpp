#include <algorithm>
#include <cstring>
#include <cstdio>

#include "UsbRomDll.hpp"
#include "UsbRomDrv.hpp"
#include "../Amldbglog/Amldbglog.h"
#include "../AmlTime.h"
#include "../defs.h"


int AmlUsbWriteLargeMem (AmlUsbRomRW *rom) {
  static int WriteSeqNum = 0;

  if (ValidParamDWORD(&rom->bufferLen) != 1) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) != 1) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) != 1) {
    return -3;
  }

  struct AmlUsbDrv drv = {.device = rom->device};
  if (OpenUsbDevice(&drv) != 1) {
    return -4;
  }

  unsigned int bufferPtr = 0;
  int startTransfer = -1;
  int transferErrorCnt = 0;
  int retry = 0;
  unsigned short checksum = ::checksum((unsigned short *) rom->buffer, rom->bufferLen);
  while (true) {
    unsigned int bufferRemain = rom->bufferLen;
    if (++retry == 4) {
      break;
    }
    while (bufferRemain > 0) {
      unsigned int transferSize = std::min(bufferRemain, 0x1000u);
      if (startTransfer == -1) {
        unsigned int maxAllowedSize = std::min(bufferRemain, 0x10000u);
        if (WriteLargeMemCMD(&drv, rom->address, maxAllowedSize, transferSize, checksum,
                              WriteSeqNum) == 0) {
          break;
        }
        startTransfer = maxAllowedSize;
      }
      ++WriteSeqNum;
      int actual_len = write_bulk_usb(&drv, (char *) &rom->buffer[bufferPtr],
                                      transferSize);
      if (actual_len) {
        transferErrorCnt = 0;
      } else if (++transferErrorCnt > 5) {
        goto finish;
      }
      if (actual_len == -1) {
        goto finish;
      }
      bufferPtr += actual_len;
      bufferRemain -= actual_len;
    }
    if (bufferRemain == 0) {
      break;
    }
  }

  finish:
  CloseUsbDevice(&drv);
  *rom->pDataSize = bufferPtr;
  return rom->bufferLen == bufferPtr ? 0 : -6;
}


int AmlUsbReadLargeMem (AmlUsbRomRW *rom) {
  static int ReadSeqNum = 0;

  if (ValidParamDWORD(&rom->bufferLen) != 1) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) != 1) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) != 1) {
    return -3;
  }

  ++ReadSeqNum;
  struct AmlUsbDrv drv = {.device = rom->device};
  if (OpenUsbDevice(&drv) != 1) {
    return -4;
  }

  unsigned int bufferPtr = 0;
  int startTransfer = -1;
  int transferErrorCnt = 0;
  int retry = 0;
  unsigned short checksum = ::checksum((unsigned short *) rom->buffer, rom->bufferLen);
  while (true) {
    if (++retry == 4) {
      break;
    }
    unsigned int bufferRemain = rom->bufferLen;
    while (bufferRemain > 0) {
      unsigned int transferSize = std::min(bufferRemain, 0x10000u);
      if (startTransfer == -1) {
        if (ReadLargeMemCMD(&drv, rom->address, rom->bufferLen,
                            transferSize >= 0x1000 ? 0x1000 : std::min(transferSize, 0x200u),
                            checksum, ReadSeqNum) == 0) {
          break;
        }
        startTransfer = transferSize;
      }
      int actual_len = read_bulk_usb(&drv, (char *) &rom->buffer[bufferPtr],
                                      transferSize);
      if (actual_len) {
        transferErrorCnt = 0;
      } else if (++transferErrorCnt > 5) {
        goto finish;
      }
      if (actual_len == -1) {
        goto finish;
      }
      bufferPtr += actual_len;
      bufferRemain -= actual_len;
    }
    if (bufferRemain == 0) {
      break;
    }
  }

  finish:
  CloseUsbDevice(&drv);
  *rom->pDataSize = bufferPtr;
  return rom->bufferLen == bufferPtr ? 0 : -6;
}


int AmlUsbReadMemCtr (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};
  if (OpenUsbDevice(&drv) == 0) {
    return 0;
  }

  unsigned int bufferRemain = rom->bufferLen;
  char *bufferPtr = rom->buffer;
  while (bufferRemain > 0) {
    unsigned int transferSize = std::min(rom->bufferLen, 64u);
    if (read_control_usb(&drv, rom->address, bufferPtr, transferSize) != 1) {
      break;
    }
    rom->address += transferSize;
    bufferPtr += transferSize;
    bufferRemain -= transferSize;
  }

  CloseUsbDevice(&drv);
  *rom->pDataSize = *(unsigned int *) bufferPtr - *(unsigned short *) (rom->buffer);
  return bufferRemain ? -1 : 0;
}


int AmlUsbWriteMemCtr (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};
  if (OpenUsbDevice(&drv) == 0) {
    return 0;
  }

  unsigned int bufferRemain = rom->bufferLen;
  char *bufferPtr = rom->buffer;
  while (bufferRemain > 0) {
    unsigned int transferSize = std::min(rom->bufferLen, 64u);
    if (write_control_usb(&drv, rom->address, bufferPtr, transferSize) == 0) {
      break;
    }
    rom->address += transferSize;
    bufferPtr += transferSize;
    bufferRemain -= transferSize;
  }

  CloseUsbDevice(&drv);
  return bufferRemain == 0;
}


int AmlUsbRunBinCode (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};

  aml_printf("AmlUsbRunBinCode:ram_addr=%08x\n", rom->address);
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControl(&drv, 0x8000200C, &rom->address, 4, nullptr, 0, nullptr,
                               nullptr);
  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlUsbIdentifyHost (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};

  aml_printf("AmlUsbIdentifyHost\n");
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }

  int ret = usbDeviceIoControl(&drv, 0x80002024, nullptr, 0, rom->buffer, rom->bufferLen,
                               rom->pDataSize, nullptr);
  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlUsbTplCmd (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};

  aml_printf("AmlUsbTplCmd = %s ", rom->buffer);
  if (OpenUsbDevice(&drv) == 0) {
    return -1;
  }


  int ret = usbDeviceIoControl(&drv, 0x80002040, rom->buffer, rom->bufferLen, nullptr, 0,
                               rom->pDataSize, nullptr);
  aml_printf("rettemp = %d buffer = %s\n", ret, rom->buffer);
  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlUsbburn (struct usb_device *device, const char *filename, unsigned int address,
                const char *memType, int nBytes, size_t bulkTransferSize, int checksum) {
  if (!device) {
    return -15;
  }

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return -25;
  }

  AmlUsbRomRW rom = {.device = device, .address = address};
  auto *buffer = (char *) malloc(bulkTransferSize);
  fseek(fp, 0, 2);
  int filePtr = 0;
  int ret;
  auto len = (size_t) ftell(fp);
  fseek(fp, 0, 0);
  while (len) {
    size_t transferSize = std::min(len, bulkTransferSize);
    fread(buffer, 1, transferSize, fp);
    rom.buffer = buffer;
    rom.bufferLen = transferSize;
    unsigned int dataSize;
    rom.pDataSize = &dataSize;
    ret = AmlUsbBurnWrite(&rom, (char *) memType, nBytes, checksum);
    if (ret) {
      break;
    }
    len -= dataSize;
    rom.address += dataSize;
    filePtr += dataSize;
    fseek(fp, filePtr, 0);
  }
  if (buffer) {
    free(buffer);
  }
  if (fp) {
    fclose(fp);
  }
  return ret ? ret : filePtr;
}


int AmlUsbReadStatus (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};
  aml_printf("AmlUsbReadStatus ");
  if (OpenUsbDevice(&drv) != 1) {
    return -1;
  }

  int retusb = usbDeviceIoControl(&drv, 0x80002044, rom->buffer, 4, rom->buffer,
                                  rom->bufferLen, rom->pDataSize, nullptr);
  aml_printf("retusb = %d\n", retusb);
  CloseUsbDevice(&drv);
  return retusb == 1 ? 0 : -2;
}


int AmlUsbReadStatusEx (AmlUsbRomRW *rom, unsigned int timeout) {
  struct AmlUsbDrv drv = {.device = rom->device};
  aml_printf("AmlUsbReadStatus ");
  if (OpenUsbDevice(&drv) != 1) {
    return -1;
  }

  int retusb = usbDeviceIoControlEx(&drv, 0x80002044, rom->buffer, 4, rom->buffer,
                                    rom->bufferLen, rom->pDataSize, nullptr, timeout);
  aml_printf("retusb = %d\n", retusb);
  CloseUsbDevice(&drv);
  return retusb == 1 ? 0 : -2;
}


int AmlResetDev (AmlUsbRomRW *rom) {
  if (!rom->device) {
    return -1;
  }

  struct AmlUsbDrv drv = {.device = rom->device};
  if (OpenUsbDevice(&drv) != 1) { return 2; }

  aml_printf("reset worldcup device\n");
  int result = ResetDev(&drv);
  CloseUsbDevice(&drv);
  return result;
}


int AmlSetFileCopyComplete (AmlUsbRomRW *rom) {
  aml_printf("%s L%d not implemented", "AmlSetFileCopyComplete", 596);
  return 0;
}


int AmlGetUpdateComplete (AmlUsbRomRW *rom) {
  aml_printf("%s L%d not implemented", "AmlGetUpdateComplete", 662);
  return 0;
}


int AmlSetFileCopyCompleteEx (AmlUsbRomRW *rom) {
  aml_printf("%s L%d not implemented", "AmlSetFileCopyCompleteEx", 730);
  return 0;
}


int AmlWriteMedia (AmlUsbRomRW *rom) {
  int result = 0;
  unsigned int checksum = 0;

  struct AmlUsbDrv drv = {.device = rom->device};

  if (ValidParamDWORD(&rom->bufferLen) != 1) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) != 1) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) != 1) {
    return -3;
  }

  if (OpenUsbDevice(&drv) != 1) {
    aml_printf("Open device failed\n");
    return -4;
  }

  checksum = checksum_64K(rom->buffer, rom->bufferLen);

  for (int address = 0; address <= 2; ++address) {
    unsigned int want_write = std::min(rom->bufferLen, 0x10000u);
    unsigned int cmd = 16 * rom->address;
    if (WriteMediaCMD(&drv, address, rom->bufferLen, checksum, cmd, want_write, 5000) !=
        1) {
      aml_printf("Write media command %d failed\n", cmd);
      CloseUsbDevice(&drv);
      return -6;
    }
    unsigned int actual_len = 0;
    int ret = usbWriteFile(&drv, rom->buffer, want_write, &actual_len);
    if (ret != 1) {
      aml_printf("usbReadFile failed ret=%d", ret);
      CloseUsbDevice(&drv);
      return -7;
    }
    if (want_write != actual_len) {
      aml_printf("[AmlUsbRom]Err:");
      aml_printf("Want Write 0x%x, but actual_len 0x%x\n", want_write, actual_len);
      result = -797;
      break;
    }
    result = 0;
    unsigned char buf[512] = {};
    for (time_t startTime = GetTickCount(), curTime = startTime;
         curTime - startTime < 12 * 60 * 1000; curTime = GetTickCount()) {
      if (usbReadFile(&drv, buf, sizeof(buf), &actual_len) != 1) {
        aml_printf("[AmlUsbRom]Err:");
        aml_printf("usbReadFile failed ret=%d", 0);
        result = -809;
        break;
      }
      char strBusy[] = "Continue:32";
      size_t strLenBusy = strlen(strBusy);
      if (strLenBusy > actual_len) {
        aml_printf("[AmlUsbRom]Err:");
        aml_printf("return size %d < strLenBusy %d\n", actual_len, strLenBusy);
        result = -814;
        break;
      }
      if (strncmp((const char *) buf, strBusy, strLenBusy) != 0) {
        break;
      }
      usleep(500000);
    }
    if (!result) {
      result = strncmp((const char *) buf, "OK!!", 4);
      if (result) {
        continue;
      }
    }
    break;
  }

  CloseUsbDevice(&drv);
  return result;
}


int AmlReadMedia (AmlUsbRomRW *rom) {
  struct AmlUsbDrv drv = {.device = rom->device};
  unsigned char buf[200] = {};
  unsigned int len = rom->bufferLen;
  unsigned int read = 0;
  unsigned int checksum = 0;

  if (ValidParamDWORD(&rom->bufferLen) != 1) {
    return -1;
  }
  if (ValidParamHANDLE((void **) &rom->device) != 1) {
    return -2;
  }
  if (ValidParamVOID(rom->buffer) != 1) {
    return -3;
  }

  if (OpenUsbDevice(&drv) != 1) {
    aml_printf("Open device failed\n");
    return -4;
  }

  if (ReadMediaCMD(&drv, 0, len, checksum, 0, 0, 5000) != 1) {
    aml_printf("Read media command failed\n", 0);
    CloseUsbDevice(&drv);
    return -5;
  }

  memset(rom->buffer, 0, rom->bufferLen);
  int ret = usbReadFile(&drv, rom->buffer, len, &read);
  if (ret == 0) {
    aml_printf("usbReadFile failed ret=%d\n", ret);
    CloseUsbDevice(&drv);
    return -6;
  }

  if (len != read) {
    aml_printf("Want read %d bytes, actual len %d\n", rom->bufferLen, read);
    CloseUsbDevice(&drv);
    return -7;
  }

  *rom->pDataSize = read;
  CloseUsbDevice(&drv);
  return 0;
}


int AmlUsbBulkCmd (AmlUsbRomRW *rom) {
  AmlUsbDrv drv = {.device = rom->device, .read_ep = 2};

  if (OpenUsbDevice(&drv) == 0) {
    aml_printf("[AmlUsbRom]Err:");
    aml_printf("FAil in OpenUsbDevice\n");
    return -924;
  }

  aml_printf("AmlUsbBulkCmd[%s]\n", rom->buffer);
  if (!usbDeviceIoControlEx(&drv, 0x80002050, rom->buffer, rom->bufferLen, nullptr, 0,
                            rom->pDataSize, nullptr, 5000)) {
    aml_printf("[AmlUsbRom]Err:");
    aml_printf("rettemp = %d buffer = [%s]\n", 0, rom->buffer);
    CloseUsbDevice(&drv);
    return -2;
  }

  char buf[512] = {};
  bool success = true;
  for (time_t startTime = GetTickCount(), curTime = startTime;
       curTime - startTime < 20 * 60 * 1000; curTime = GetTickCount()) {
    int ret = read_bulk_usb(&drv, buf, sizeof(buf));
    char strBusy[] = "Continue:34";
    size_t strLenBusy = strlen(strBusy);
    if (strLenBusy > ret) {
      aml_printf("[AmlUsbRom]Err:", buf);
      aml_printf("return len=%d < strLenBusy %d\n", ret, strLenBusy);
      success = false;
      break;
    }
    if (strncmp(buf, strBusy, strLenBusy) != 0) {
      break;
    }
    usleep(3000000);
  }
  if (success) {
    success = strncmp(buf, "success", 7) == 0;
  }
  if (!success) {
    aml_printf("[AmlUsbRom]Err:");
    aml_printf("bulkInReply=[%s]\n", buf);
  }
  CloseUsbDevice(&drv);
  return success ? 0 : -2;
}


int AmlUsbCtrlWr (AmlUsbRomRW *rom) {
  AmlUsbDrv drv = {.device = rom->device, .read_ep = 2};

  if (OpenUsbDevice(&drv) != 1) {
    return -1;
  }
  int ret = usbDeviceIoControlEx(&drv, 0x80002004, rom->buffer, rom->bufferLen, nullptr,
                                 0, rom->pDataSize, nullptr, 5000);
  CloseUsbDevice(&drv);
  return ret == 1 ? 0 : -2;
}


int AmlUsbBurnWrite (AmlUsbRomRW *cmd, char *memType, unsigned long long nBytes,
                     int checksum) {
  unsigned int oldDataSize = *cmd->pDataSize;

  int ret = AmlUsbWriteLargeMem(cmd);
  if (ret) {
    return ret;
  }

  char buf[88] = {};
  SET_INT_AT(buf, 0, cmd->address);
  SET_INT_AT(buf, 4, 0);
  SET_INT_AT(buf, 8, checksum);
  SET_INT_AT(buf, 24, cmd->bufferLen);
  SET_INT_AT(buf, 28, checksum);
  SET_LONG_AT(buf, 32, nBytes);
  memcpy(&buf[48], memType, strlen(memType));
  SET_INT_AT(buf, 80, 0);
  cmd->bufferLen = 88;
  cmd->buffer = buf;
  ret = AmlUsbTplCmd(cmd);
  if (ret) {
    *cmd->pDataSize = 0;
  } else {
    *cmd->pDataSize = oldDataSize;
  }
  return ret;
}
