#include <algorithm>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <unistd.h>

#include "defs.h"
#include "gettime.h"

#include "Amldbglog.h"
#include "AmlLibusb.h"
#include "AmlUsbScanX3.h"
#include "UsbRomDrv.h"
#include "scan.h"


#define AML_CHIP_ID_LEN 12


static bool simg_probe (const unsigned char *buf, unsigned int len) {
  const unsigned char magic[] = {0x3A, 0xFF, 0x26, 0xED};
  const unsigned char magic2[] = {0x28, 0, 0x12, 0};

  if (len < 28 || memcmp(buf, magic, 4) != 0 || GET_U16(&buf[4]) != 1 ||
      memcmp(&buf[8], magic2, 4) != 0) {
    return false;
  }

  aml_printf("[update]sparse format detected\n");
  return true;
}


static bool is_file_format_sparse (const char *filename) {
  FILE *stream = fopen(filename, "rb");

  if (stream == NULL) {
    aml_printf("[update]ERR(L%d):", 68);
    aml_printf("Fail to open file in mode rb\n");
    return false;
  }

  unsigned char *buf = new unsigned char[0x2000];
  size_t len = fread(buf, 1, 0x2000, stream);
  fclose(stream);

  bool ret = simg_probe(buf, (unsigned int) len);

  if (buf != NULL) {
    delete[] buf;
  }
  return ret;
}


#ifndef BUGFIX
struct DownloadProgressInfo {
  long nBytes;
  int percentageBegin;
  int percentageEnd;
  int percentageRange;
  int percentage;
  unsigned long unshownBytes;
  unsigned int startTime;
  unsigned int percentBytes;
  unsigned int updateBytes;
  unsigned int updatePercentBytes;
  char prompt[16];

  DownloadProgressInfo (long nBytes_, const char *prompt_);
  int update_progress (unsigned int len);
};


DownloadProgressInfo::DownloadProgressInfo (long nBytes_, const char *prompt_) {
  nBytes = nBytes_;
  percentageBegin = 0;
  percentageEnd = 100;
  percentage = 0;
  unshownBytes = 0;
  percentageRange = percentageEnd - percentageBegin;
  memset(prompt, 0, sizeof(prompt));
  memcpy(prompt, prompt_, sizeof(prompt) - 1);
  if (nBytes < percentageRange) {
    nBytes = percentageRange;
  }
  percentBytes = nBytes / percentageRange;
  updateBytes = aml_max(percentBytes, 0x400000);
  updatePercentBytes = updateBytes / percentBytes;
}


int DownloadProgressInfo::update_progress (unsigned int len) {
  unshownBytes += len;
  if (unshownBytes == len && percentageBegin == percentage) {
    startTime = timeGetTime();
  }

  if (unshownBytes < updateBytes) {
    return 0;
  }

  percentage += unshownBytes / percentBytes;
  unshownBytes %= percentBytes;
  printf("%s %%%d\r", prompt, percentage);
  fflush(stdout);
  if (updatePercentBytes + percentage >= percentageEnd) {
    printf("\b\b\b\b\b\b\b\b\b\r");
    printf(
      "[%s]OK:<%ld>MB in <%u>Sec\n", prompt, nBytes >> 20,
      (unsigned int) ((timeGetTime() - startTime) / 1000));
  }
  return 0;
}
#else
struct DownloadProgressInfo {
  size_t nBytes;
  time_t startTime;
  size_t unshownBytes;
  size_t transferSize;
  size_t updateBytes;
  char prompt[16];

  DownloadProgressInfo (size_t nBytes_, const char *prompt_);
  void update_progress (size_t len);
};


DownloadProgressInfo::DownloadProgressInfo (
    size_t nBytes_, const char *prompt_) {
  nBytes = nBytes_;
  startTime = 0;
  unshownBytes = 0;
  transferSize = 0;
  updateBytes = aml_max(nBytes / 100, 0x400000);
  memset(prompt, 0, sizeof(prompt));
  memcpy(prompt, prompt_, sizeof(prompt) - 1);
}


void DownloadProgressInfo::update_progress (size_t len) {
  if (transferSize == 0) {
    startTime = timeGetTime();
  }

  unshownBytes += len;
  transferSize += len;
  if (unshownBytes < updateBytes && transferSize < nBytes) {
    return;
  }

  unshownBytes = 0;
  if (transferSize < nBytes) {
    printf("%s %%%d\r", prompt, (int) (100 * transferSize / nBytes));
    fflush(stdout);
  } else {
    printf(
      "[%s]OK:<%ld>MB in <%u>Sec\n", prompt, nBytes >> 20,
      (unsigned int) ((timeGetTime() - startTime) / 1000));
  }
}
#endif


#ifndef BUGFIX
static std::string strToLower (const std::string &src) {
  std::string dst(src);
  std::transform(src.begin(), src.end(), dst.begin(), tolower);
  return dst;
}
#endif


static unsigned long simple_strtoul (
    const char *cp, char **endp, unsigned int base) {
#ifndef BUGFIX
  if (cp[0] == '0') {
    cp++;
    if (cp[1] == 'x' && isxdigit(cp[2])) {
      base = 16;
      cp += 2;
    }
    if (base == 0) {
      base = 8;
    }
  }
  if (base == 0) {
    base = 10;
  }

  unsigned long result = 0;
  for (; isxdigit(*cp); cp++) {
    unsigned int d;
    if (*cp - '0' > 9) {
      d = (islower(*cp) ? toupper(*cp) : *cp) - '7';
    } else {
      d = *cp - '0';
    }
    if (d >= base) {
      break;
    }
    result = base * result + d;
  }

  if (endp) {
    *endp = (char *) cp;
  }
  return result;
#else
  return strtoul(cp, endp, base);
#endif
}


static void _print_memory_view (
    char *buf, unsigned int size, unsigned int addr) {
  for (unsigned int line = 0; line < (size + 15) >> 4; line++) {
    printf("\n%08X: ", addr);
    unsigned int len = 16 * (line + 1) <= size ? 16 : size & 0xF;
    for (unsigned int col = 0; col < (len + 3) >> 2; col++) {
      printf("%08x ", *(unsigned int *) &buf[4 * (4 * line + col)]);
    }
    addr += 16;
  }
  putchar('\n');
}


static int WriteMediaFile (struct AmlUsbRomRW *rom, const char *filename) {
  FILE *stream = fopen(filename, "rb");
  if (stream == NULL) {
    aml_printf("Open file %s failed\n", filename);
    return -1;
  }

  fseeko(stream, 0, SEEK_END);
  unsigned long long fileSize = ftello(stream);
  unsigned long long percentSize = fileSize / 100;
  unsigned long long updateSize = aml_max(fileSize / 100, 0x400000u);
  fseek(stream, 0, SEEK_SET);

  time_t startTime = timeGetTime();
  char *buf = (char *) malloc(0x10000);

  unsigned int addr = 0;
  unsigned long long transferSize = 0;
  unsigned long long shownSize = 0;
  unsigned long long remain = fileSize;
  while (remain != 0) {
    unsigned int bulkSize = aml_min(remain, 0x10000u);
    fread(buf, 1, bulkSize, stream);

    unsigned int actualLen;
    rom->buffer = buf;
    rom->bufferLen = bulkSize;
    rom->pDataSize = &actualLen;
    rom->address = addr;
    if (AmlWriteMedia(rom) != 0) {
      aml_printf("AmlWriteMedia failed\n");
      break;
    }

    transferSize += bulkSize;
    remain -= bulkSize;
    addr++;
    if (addr == 1) {
      puts("Downloading....");
    }

    if (transferSize - shownSize >= updateSize) {
      int percentage = 100 * transferSize / fileSize;
      printf(
        "[%3d%%/%5uMB]\r", percentage, (unsigned int) (transferSize >> 20));
      fflush(stdout);
      shownSize = percentSize * percentage;
    }
  }

  aml_printf(
    "[update]Cost time %dSec            \n",
    (int) (timeGetTime() - startTime) / 1000);
  aml_printf(
    "[update]Transfer size 0x%llxB(%lluMB)\n", transferSize,
    transferSize >> 20);
  free(buf);
  fclose(stream);
  return remain == 0 ? 0 : -1;
}


static int ReadMediaFile (
    struct AmlUsbRomRW *rom, const char *filename, long size) {
  FILE *stream = NULL;
  if (filename) {
    stream = fopen(filename, "wb");
    if (stream == NULL) {
      aml_printf("Open file %s failed\n", filename);
      return -1;
    }
  }

  char *buf = (char *) malloc(0x10000);
  DownloadProgressInfo info(size, "Uploading");
  unsigned int offset = 0;
  while (size != 0) {
    unsigned int actualLen;
    rom->buffer = buf;
    rom->bufferLen = aml_min(size, 0x10000u);
    rom->pDataSize = &actualLen;
    if (AmlReadMedia(rom) != 0) {
      aml_printf("AmlReadMedia failed\n");
      break;
    }
    if (filename) {
      fwrite(buf, 1, actualLen, stream);
      info.update_progress(actualLen);
    } else {
      _print_memory_view(buf, actualLen, offset);
    }

    offset += actualLen;
    size -= actualLen;
  }

  if (buf != NULL) {
    free(buf);
  }
  if (stream != NULL) {
    fclose(stream);
  }
  return size != 0 ? -1 : 0;
}


static void update_help () {
  puts("====>Amlogic update USB tool(Ver 1.7.2) 2018/04<=============");
  puts(
    "update\t<command>\t[device name]\t<arg0>\t<arg1>\t<arg2>\t...\n"
    "\nCommon Commands:\n"
    "update <partition>: Burn a partition with a partition image\n"
    "update <mwrite>   : Burn data to media or memory\n"
    "update <mread>    : Dump a data from media or memory to pc and save as a file\n"
    "update <tplcmd>   : like bulkcmd\n"
    "update <bulkcmd>  : pass and exec a command platform bootloader can support\n"
    "update <write>    : Down a file to memory\n"
    "update <run>      : Run code from memory address\n"
    "update <read>     : Dump data from memory:\n"
    "update <wreg>     : set one 32bits reg:\n"
    "update <rreg>     : Dump data from reg:\n"
    "update <password> : unlock chip:\n"
    "update <chipinfo> : get chip info at page index:\n"
    "update <chipid>   : get chip id\n"
    "update <bl2_boot> : boot fip format u-boot.bin\n"
    "\nCommon Commands format:\n"
    "update partition partName imgFilePath [imgFileFmt] [sha1VeryFile]\n"
    "\t\te.g.--\tupdate partition boot z:\\a\\b\\boot.img [normal] //format normal is optional\n"
    "\t\te.g.--\tupdate partition system z:\\xxxx\\system.img [sparse] //format sparse is optional\n"
    "\t\te.g.--\tupdate partition upgrade z:\\xxxx\\upgrade.ubifs.img ubifs //format ubifs is MANDATORY\n"
    "\nupdate bulkcmd \"burning cmd or u-boot cmd\"\n"
    "\t\te.g.--\tupdate bulkcmd \"disk_intial 0\" //cmd to init flash for usb burning\n"
    "\t\te.g.--\tupdate bulkcmd \"printenv\" //uboot command 'printenv'\n"
    "\nupdate mread store/mem partName/memAddress\n"
    "\t\te.g.--\tupdate mread store boot normal 0x200000 d:\boot.dump //upload 32M of boot partition in path d:\boot.dump\n"
    "\t\te.g.--\tupdate mread mem 0x1080000 normal d:\\mem_2M.dump //upload 2M memory at address 0x1080000 in path d:\\mem_2M.dump\n"
    "\t\te.g.--\tupdate chipinfo pageIndex dumpFilePath nBytes startOffset\n"
    "\nupdate cwr/write filePath memAddress [nBytes] [fileOffset]\n"
    "\t\te.g.--\tupdate cwr/write z:/u-boot.bin 0xffa0000 0x10000\n"
    "\t\te.g.--\tupdate cwr/write z:/u-boot.bin.usb.bl2 0xffa0000\n"
    "\t\te.g.--\tupdate write z:/u-boot.bin.usb.tpl 0x200c000\n"
    "\t\te.g.--\tupdate write z:/u-boot.bin 0x200c000 0 0xc000");
  puts("====>Amlogic update USB tool(Ver 1.7.2) 2018/04<=============");
}


static int update_scan (
    void **result, int print_devices, int dev_no, int *success,
    char *scan_mass_storage) {
  int ret = 0;
  *success = -1;

  char *buf = new char[0x800];
  memset(buf, 0, 0x800);

  char *candidateDevices[16] = {};
  candidateDevices[0] = buf;
  for (int i = 1; i <= 15; ++i) {
    candidateDevices[i] = candidateDevices[i - 1] + 128;
  }

  const char *vendorName =
    scan_mass_storage ? "USB Mass Storage Device" : "WorldCup Device";
  int nDevices = AmlScanUsbX3Devices(vendorName, candidateDevices);
  if (nDevices <= 0) {
    aml_printf("[update]No [%s] device after scan\n", vendorName);
    ret = -182;
    goto end;
  }

  if (dev_no != -2 && nDevices) {
    if (dev_no + 1 > nDevices) {
      aml_printf("[update]ERR(L%d):", 190);
      aml_printf(
        "dev_no(%d) too only, only [%d] (%s)devices Connected\n", dev_no,
        nDevices, vendorName);
      ret = -191;
      goto end;
    }
    if (!scan_mass_storage) {
      if (!result) {
        aml_printf("[update]ERR(L%d):", 202);
        aml_printf("handle for (%s) NULL is invalid\n", vendorName);
        ret = -203;
        goto end;
      }
      *result = AmlGetDeviceHandle(vendorName, candidateDevices[dev_no]);
    } else {
      printf("%d.%s ==> ", dev_no, candidateDevices[dev_no]);
      AmlGetMsNumber(candidateDevices[dev_no], 1, scan_mass_storage);
    }
  }

  *success = 0;
  if (print_devices) {
    printf(
      "\t-------%sdevices list -------\n",
      scan_mass_storage ? "Mass Storage " : "MPtool ");
    for (int i = 0; i < nDevices; ++i) {
      printf("WorldCup[%02d].%s\n", i, candidateDevices[i]);
    }
  }

end:
  if (buf != NULL) {
    delete[] buf;
  }
  return ret;
}


static int do_cmd_mwrtie (
    const char **argv, int argc, struct AmlUsbRomRW &rom) {
  FILE *stream = NULL;
  int ret = 0;

  do {
    const char *readFile = argv[0];
    stream = fopen(readFile, "rb");
    if (stream == NULL) {
      aml_printf("Open file %s failed\n", readFile);
      ret = -236;
      goto end;
    }

    fseeko(stream, 0, SEEK_END);
    off_t fileSize = ftello(stream);
    fclose(stream);
    stream = NULL;
    aml_printf("file size is 0x%llx\n", (unsigned long long) fileSize);
    if (!fileSize) {
      aml_printf("file size 0!!\n");
      return -103;
    }

    char buf[128] = {};
    snprintf(
      buf, sizeof(buf), "download %s %s %s 0x%lX", argv[1], argv[2], argv[3],
      (long) fileSize);
    buf[66] = 1;

    unsigned int actualLen;
    rom.buffer = buf;
    rom.bufferLen = 68;
    rom.pDataSize = &actualLen;
    if (AmlUsbTplCmd(&rom) != 0) {
      goto end;
    }

    memset(buf, 0, 0x80);
    int retry;
    for (retry = 1; retry != 0; retry--) {
      rom.buffer = buf;
      rom.bufferLen = 64;
      rom.pDataSize = &actualLen;
      if (AmlUsbReadStatus(&rom) == 0) {
        break;
      }
      usleep(1000000);
    }
    if (retry == 0) {
      aml_printf("Read status failed\n");
      ret = -300;
      goto end;
    }

    if (strncmp(rom.buffer, "success", 7) != 0) {
      aml_printf("[update]ERR(L%d):", 306);
      aml_printf("cmdret=[%s]\n", rom.buffer);
      ret = -307;
      goto end;
    }

    if (WriteMediaFile(&rom, readFile) != 0) {
      aml_printf("ERR:write data to media failed\n");
      ret = -314;
      goto end;
    }

    strcpy(buf, "download get_status");
    buf[66] = 1;

    rom.buffer = buf;
    rom.bufferLen = 68;
    rom.pDataSize = &actualLen;
    if (AmlUsbBulkCmd(&rom) != 0) {
      aml_printf("[update]ERR(L%d):", 327);
      aml_printf("AmlUsbBulkCmd failed!\n");
      ret = -328;
      goto end;
    }

    const char *verifyFile = argc <= 4 ? NULL : argv[4];
    if (verifyFile != NULL) {
      memset(buf, 0, 0x80);
      stream = fopen(verifyFile, "rb");
      if (stream == NULL) {
        aml_printf("Open file %s failed\n", verifyFile);
        goto end;
      }

      strcpy(buf, "verify ");
      fread(&buf[7], 1, 0x79, stream);
      fclose(stream);
      stream = NULL;
      buf[66] = 1;

      rom.buffer = buf;
      rom.bufferLen = 68;
      rom.pDataSize = &actualLen;
      if (AmlUsbBulkCmd(&rom) != 0) {
        aml_printf("ERR: AmlUsbBulkCmd failed!\n");
        ret = -354;
        goto end;
      }
    }
  } while (0);

  ret = 0;
  aml_printf("[update]mwrite success\n");

end:
  if (stream != NULL) {
    fclose(stream);
  }
  if (rom.device != NULL) {
    rom.device = NULL;
  }
  return ret;
}


static int update_sub_cmd_run_and_rreg (
    struct AmlUsbRomRW &rom, const char *cmd, const char **argv, int argc) {
  if (argc <= 0) {
    aml_printf("[update]ERR(L%d):", 381);
    aml_printf(
      "argc(%d)<2, address for cmd[%s] not assigned\n",
#ifndef BUGFIX
      argv,
#else
      argc,
#endif
      cmd);
    return -382;
  }

  rom.address =
#ifndef BUGFIX
    simple_strtoul(strToLower(argv[0]).c_str(), NULL, 0);
#else
    strtoul(argv[0], NULL, 0);
#endif
  if (strcmp(cmd, "run") == 0) {
    aml_printf("[update]Run at Addr %x\n", rom.address);
    rom.buffer = (char *) &rom.address;
    int ret = AmlUsbRunBinCode(&rom);
    if (ret) {
      puts("ERR: Run cmd failed");
    }
    return ret;
  }

  unsigned int addr =
#ifndef BUGFIX
    simple_strtoul(strToLower(argv[1]).c_str(), NULL, 0);
#else
    strtoul(argv[1], NULL, 0);
#endif
  const char *saveFile = argc <= 2 ? NULL : argv[2];
  FILE *saveStream = saveFile == NULL ? NULL : fopen(saveFile, "wb");
  char *buf = new char[0x100000];

  bool isRreg = strcmp(cmd, "rreg") == 0;
  FILE *readStream = NULL;
  unsigned int transferSize;
  if (isRreg) {
    transferSize = rom.address;
  } else {
    readStream = fopen(argv[0], "rb");
    fseeko(readStream, 0, SEEK_END);
    transferSize = ftell(readStream);
    fseek(readStream, 0, SEEK_SET);
  }
  aml_printf("[update]Total tansfer size 0x%x\n", transferSize);

  int ret = 0;
  unsigned int offset = 0;
  while (offset < transferSize) {
    unsigned int bulkSize = aml_min(transferSize - offset, 0x100000u);
    if (readStream != NULL) {
      fread(buf, bulkSize, 1, readStream);
    }

    ret = Aml_Libusb_Ctrl_RdWr(rom.device, addr, buf, bulkSize, isRreg, 5000);
    if (ret) {
      aml_printf("[update]ret=%d\n", ret);
      break;
    }

    if (isRreg) {
      if (saveStream != NULL) {
        fwrite(buf, bulkSize, 1, saveStream);
      } else {
        _print_memory_view(buf, bulkSize, addr);
      }
    }
    offset += bulkSize;
    addr += bulkSize;
  }

  if (buf != NULL) {
    delete[] buf;
  }
  if (saveStream != NULL) {
    fclose(saveStream);
  }
  return ret;
}


static int update_sub_cmd_set_password (
    struct AmlUsbRomRW &rom, const char **argv, int argc) {
  (void) argc;

  const char *pwdFile = argv[0];
  FILE *stream = fopen(pwdFile, "rb");
  if (stream == NULL) {
    aml_printf("[update]ERR(L%d):", 478);
    aml_printf("Open file %s failed\n", pwdFile);
    return -1;
  }

  fseeko(stream, 0, SEEK_END);
  size_t totalFileSz = ftello(stream);
  if (totalFileSz > 64) {
    aml_printf("[update]ERR(L%d):", 485);
    aml_printf(
      "size of file(%s) is %lld too large!!\n", pwdFile,
      (long long) totalFileSz);
    fclose(stream);
    return -487;
  }

  fseek(stream, 0, SEEK_SET);
  char buf[64];
  size_t rdLen = fread(buf, 1, totalFileSz, stream);
  if (rdLen != totalFileSz) {
    aml_printf("[update]ERR(L%d):", 494);
    aml_printf(
      "FAil in read pwd file (%s), rdLen(%d) != TotalFileSz(%d)\n", pwdFile,
      (int) rdLen, (int) totalFileSz);
    fclose(stream);
    return -496;
  }

  fclose(stream);
  int ret = 0;
  for (int i = 0; i <= 1 && (ret & 0x80000000) == 0; ++i) {
    ret = Aml_Libusb_Password(rom.device, buf, rdLen, 8000);
    if (!i) {
      printf("Setting PWD..");
      usleep(5000000);
    }
  }

  aml_printf("Pwd returned %d\n", ret);
  return ret <= 0 ? ret : 0;
}


static int update_sub_cmd_get_chipinfo (
    struct AmlUsbRomRW &rom, const char **argv, int argc) {
  int pageIndex = atoi(argv[0]);
  const char *saveFile = argc <= 1 ? "-" : argv[1];
  int nBytes = argc <= 2 ? 64 : atoi(argv[2]);
  int startOffset = argc <= 3 ? 0 : atoi(argv[3]);

  aml_printf(
    "[update]pageIndex %d, saveFile %s, nBytes %d, startOffset %d\n", pageIndex,
    saveFile, nBytes, startOffset);
  if (nBytes > 64) {
    aml_printf("[update]ERR(L%d):", 526);
    aml_printf("pageSzMax %d < nBytes %d\n", 64, nBytes);
    return -527;
  }

  char *buf = new char[0x40];
  if (!buf) {
    aml_printf("[update]ERR(L%d):", 523);
    aml_printf("Fail in alloc buff\n");
    return -524;
  }

  int ret;
  FILE *stream = NULL;

  int actualLen = Aml_Libusb_get_chipinfo(rom.device, buf, 64, pageIndex, 8000);
  if (actualLen != 64) {
    aml_printf("[update]ERR(L%d):", 537);
    aml_printf("Fail in get chipinfo, want %d, actual %d\n", 64, actualLen);
    ret = -538;
    goto end;
  }

  if (strcmp("-", saveFile) == 0) {
    _print_memory_view(&buf[startOffset], nBytes, startOffset);
    ret = actualLen;
  } else {
    stream = fopen(saveFile, "wb");
    if (stream == NULL) {
      aml_printf("[update]ERR(L%d):", 547);
      aml_printf("FAil in open file %s\n", saveFile);
      ret = -548;
      goto end;
    }

    if ((int) fwrite(&buf[startOffset], 1, nBytes, stream) != nBytes) {
      aml_printf("[update]ERR(L%d):", 553);
      aml_printf("FAil in save file %s\n", saveFile);
      ret = -554;
      goto end;
    }

    ret = 0;
  }

end:
  if (buf != NULL) {
    delete[] buf;
  }
  if (stream != NULL) {
    fclose(stream);
  }
  return ret;
}


static int update_sub_cmd_read_write (
    struct AmlUsbRomRW &rom, const char *cmd, const char **argv, int argc) {
  if (argc <= 1) {
    aml_printf("[update]ERR(L%d):", 582);
    aml_printf(
      "cmd[%s] must at least %d parameters, but only %d\n", cmd, 2, argc);
    return 583;
  }

  unsigned int addr =
#ifndef BUGFIX
    simple_strtoul(strToLower(argv[1]).c_str(), NULL, 0);
#else
    strtoul(argv[1], NULL, 0);
#endif
  rom.address = addr;

  FILE *stream = NULL;
  char *buf = NULL;
  int ret;

  if (strcmp(cmd, "wreg") == 0) {
    unsigned int val = htole32(simple_strtoul(argv[0], NULL, 16));
    rom.buffer = (char *) &val;
    rom.bufferLen = 4;
    ret = Aml_Libusb_Ctrl_RdWr(
      rom.device, rom.address, rom.buffer, rom.bufferLen, 0, 5000);
    if (ret != 0) {
      aml_printf("[update]ERR(L%d):", 601);
      aml_printf("write register failed\n");
    } else {
      aml_printf("[update]operate finished!\n");
    }
  } else if (strcmp(cmd, "read") == 0 || strcmp("dump", cmd) == 0) {
    const char *dumpFilename = argc <= 2 ? NULL : argv[2];
    FILE *dumpStream = dumpFilename ? fopen(dumpFilename, "wb") : NULL;
    unsigned int transferSize =
#ifndef BUGFIX
      simple_strtoul(strToLower(argv[0]).c_str(), NULL, 0);
#else
      strtoul(argv[0], NULL, 0);
#endif
#ifndef BUGFIX
    rom.address = simple_strtoul(strToLower(argv[1]).c_str(), 0, 0);
    addr = rom.address;
#endif
    DownloadProgressInfo info(transferSize, "DUMP");
    buf = new char[0x20000];
    while (transferSize != 0) {
      unsigned int bulkSize = aml_min(transferSize, 0x200u);
      if (strcmp("dump", cmd) == 0) {
        if (transferSize >= 0x10000) {
          bulkSize = 0x10000;
        } else if (transferSize >= 0x1000) {
          bulkSize = 0x1000;
        }
      }

      unsigned int actualLen;
      rom.buffer = buf;
      rom.bufferLen = bulkSize;
      rom.pDataSize = &actualLen;
      if (AmlUsbReadLargeMem(&rom) != 0) {
        aml_printf("[update]ERR(L%d):", 638);
        aml_printf("read device failed\n");
        ret = -639;
        goto end;
      }

      if (dumpStream == NULL) {
        _print_memory_view(buf, bulkSize, rom.address);
      } else {
        unsigned int dumpFileSize = fwrite(buf, 1, bulkSize, dumpStream);
        if (dumpFileSize != bulkSize) {
          aml_printf("[update]ERR(L%d):", 647);
          aml_printf(
            "Want to write %dB to path[%s], but only %dB\n", bulkSize,
            dumpFilename, dumpFileSize);
          ret = -648;
          break;
        }
        info.update_progress(bulkSize);
      }
      transferSize -= actualLen;
      rom.address += actualLen;
    }

    if (dumpStream != NULL) {
      fclose(dumpStream);
      dumpStream = NULL;
    }
    ret = 0;
  } else if (strcmp(cmd, "write") == 0 || strcmp(cmd, "cwr") == 0) {
    stream = fopen(argv[0], "rb");
    if (stream == NULL) {
      aml_printf("[update]ERR(L%d):", 688);
      aml_printf("ERR: can not open the %s file\n", argv[0]);
      ret = -689;
      goto end;
    }

    bool isCwr = strcmp(cmd, "write") != 0;
    buf = (char *) malloc(0x10008);

    unsigned int transferSize =
      argc <= 2 ? 0 : simple_strtoul(argv[2], NULL, 0);
    unsigned int skip = argc <= 3 ? 0 : simple_strtoul(argv[3], NULL, 0);
    if (transferSize == 0) {
      fseek(stream, 0, SEEK_END);
      transferSize = ftell(stream) - skip;
    }
    fseek(stream, skip, SEEK_SET);

    unsigned int nBytes = 0;
    while (transferSize != 0) {
      unsigned int bulkSize = aml_min(transferSize, isCwr ? 0x40 : 0x10000);
      fread(buf + 8, 1, bulkSize, stream);

      unsigned int actualLen;
      rom.buffer = buf + 4;
      rom.bufferLen = bulkSize;
      rom.pDataSize = &actualLen;

      if (isCwr) {
        SET_U32(&rom.buffer[0], rom.address);
        rom.bufferLen += 4;
        ret = AmlUsbCtrlWr(&rom);
      } else {
        rom.buffer = buf + 8;
        ret = AmlUsbWriteLargeMem(&rom);
      }
      if (ret != 0) {
        puts("ERR: write data to device failed");
        ret = -735;
        goto end;
      }

      transferSize -= bulkSize;
      rom.address += bulkSize;
      nBytes += bulkSize;
      printf("..");
    }

    printf("\nTransfer Complete! total size is %d Bytes\n", nBytes);
    ret = 0;
  } else {
    ret = 0;
  }

end:
  if (stream != NULL) {
    fclose(stream);
  }
  if (buf != NULL) {
    free(buf);
  }
  if (rom.device != NULL) {
    rom.device = NULL;
  }
  return ret;
}


static int update_sub_cmd_identify_host (
    struct AmlUsbRomRW &rom, int idLen, char *id) {
  if (idLen <= 3) {
    aml_printf("[update]ERR(L%d):", 764);
    aml_printf("idLen %d invalid\n", idLen);
    return -765;
  }

  unsigned int actualLen = 0;
  char tmp[16];
  char *buf = id == NULL ? tmp : id;

  rom.buffer = buf;
  rom.bufferLen = idLen;
  rom.pDataSize = &actualLen;
  if (AmlUsbIdentifyHost(&rom) != 0) {
    puts("ERR: get info from device failed");
    return 774;
  }

  printf("This firmware version is ");
  bool err = true;
  for (unsigned int i = 0; i < actualLen; i++) {
    printf("%d%c", buf[i], i + 1 >= actualLen ? '\n' : '-');
    if (i <= 3 && buf[i] != '\0') {
      err = false;
    }
  }
  if (err) {
    aml_printf("[update]ERR(L%d):", 783);
    aml_printf("fw ver is error! Maybe SOC wrong state\n");
    return -784;
  }

  if (buf[3] == 0) {
    if (buf[4] == 1 && idLen > 5) {
      printf("Need Password...Password check %s\n", buf[5] == 1 ? "OK" : "NG");
    }
    if (buf[7] && idLen > 7) {
      printf(
        "SupportedPageMap--\t map 6 0x%02x, map 7 0x%02x\n", buf[6], buf[7]);
    }
  }
  return 0;
}


static int update_sub_cmd_get_chipid (
    struct AmlUsbRomRW &rom, const char **argv) {
  (void) argv;

  char id[16];

  int ret = update_sub_cmd_identify_host(rom, 4, id);
  if (ret) {
    aml_printf("[update]ERR(L%d):", 854);
    aml_printf("Fail in identifyHost, ret=%d\n", ret);
    return -855;
  }

  if (id[3] != 0 && id[3] != 8) {
    aml_printf("[update]ERR(L%d):", 859);
    aml_printf("romStage not bl1/bl2, cannot support chipid\n");
    return -860;
  }

  int idVer = id[1] | (id[0] << 8);
  aml_printf("[update]idVer is 0x%x\n", idVer);

  if (idVer < 0x200 || (idVer > 0x205 && idVer != 0x300)) {
    aml_printf("[update]ERR(L%d):", 899);
    aml_printf("idVer(0x%x) cannot support chipid\n", idVer);
    return -900;
  }

  char chipID[16];

  if (idVer == 0x203) {
    aml_printf("[update]get chpid by chip info page\n");

    char buf[40];
    int actual = Aml_Libusb_get_chipinfo(rom.device, buf, sizeof(buf), 1, 8000);
    if (actual != (int) sizeof(buf)) {
      aml_printf("[update]ERR(L%d):", 891);
      aml_printf("Fail in get chipinfo, want %d, actual %d\n", 64, actual);
    }
    memcpy(chipID, &buf[20], AML_CHIP_ID_LEN);
  } else {
    unsigned int addr;
    switch (idVer) {
      case 0x200:
      case 0x201:
        addr = 0xD9013C24;
        break;
      case 0x202:
        addr = 0xC8013C24;
        break;
      case 0x204:
        addr = 0xD900D400;
        break;
      case 0x205:
      case 0x300:
        addr = 0xFFFCD400;
        break;
      default:
        addr = 0;
    }

    ret = Aml_Libusb_Ctrl_RdWr(
      rom.device, addr, chipID, AML_CHIP_ID_LEN, 1, 5000);
    if (ret != 0) {
      aml_printf("[update]ERR(L%d):", 907);
      aml_printf("Fail in read chip id via ReadMem\n");
      return -908;
    }
  }

  printf("ChipID is:0x");
  for (int i = 0; i < AML_CHIP_ID_LEN; ++i) {
    printf("%02x", (unsigned char) chipID[i]);
  }
  putchar('\n');
  return 0;
}


static int update_sub_cmd_tplcmd (struct AmlUsbRomRW &rom, const char *cmd) {
  char buf[68] = {};
  memcpy(buf, cmd, aml_min(strlen(cmd), 66));
  buf[66] = 1;

  unsigned int actualLen;
  struct AmlUsbRomRW romTpl = {rom.device, sizeof(buf), buf, &actualLen, 0};
  if (AmlUsbTplCmd(&romTpl) != 0) {
    char reply[64] = {};
    struct AmlUsbRomRW romRead = {rom.device, sizeof(reply), reply, NULL, 0};

    for (int retry = 5; retry > 0; retry--) {
      if (AmlUsbReadStatus(&romRead) == 0) {
        printf("reply %s \n", reply);
        return 0;
      }
    }
  }

  puts("ERR: AmlUsbTplCmd failed!");
  return 957;
}


static int update_sub_cmd_mread (
    struct AmlUsbRomRW &rom, int argc, const char **argv) {
  if (argc <= 3) {
    update_help();
    return 964;
  }

  long readSize = strtoll(argv[3], NULL, 0);
  if (strcmp("normal", argv[2]) != 0 || readSize == 0) {
    aml_printf("Err args in mread: check filetype and readSize\n");
    return 977;
  }

  char buf[128] = {};
  snprintf(
    buf, 67, "upload %s %s %s 0x%lx", argv[0], argv[1], argv[2],
    readSize);
  buf[66] = 1;

  rom.buffer = buf;
  rom.bufferLen = 68;
  if (AmlUsbBulkCmd(&rom) != 0) {
    aml_printf("ERR: AmlUsbBulkCmd failed!\n");
    return 988;
  }

  rom.bufferLen = readSize;
  if (ReadMediaFile(&rom, argc <= 4 ? NULL : argv[4], readSize) != 0) {
    aml_printf("ERR: ReadMediaFile failed!\n");
    return 995;
  }

  return 0;
}


int main (int argc, const char **argv) {
  aml_init();

  if (argc == 1) {
help:
    update_help();
    return 0;
  }

  const char *cmd = argv[1];
  if (strcmp(cmd, "help") == 0) {
    goto help;
  }

  char scan_mass_storage[4] = {};
  FILE *stream = NULL;
  char *buf = NULL;
  int ret = -1020;
  int success = 0;

  if (strcmp(cmd, "scan") == 0) {
    if (argc != 2) {
      if (argc != 3) {
        return 0;
      }

      if (strncmp(argv[2], "mptool", 7) != 0) {
        if (strncmp(argv[2], "msdev", 6) == 0 &&
            update_scan(NULL, 1, -2, &success, scan_mass_storage) != 0) {
          puts("can not find device");
        }
      }
    }
    update_scan(NULL, 1, -2, &success, NULL);
    return 0;
  }

  // find dev no
  int dev_no = -2;
  int cmdArgc = argc - 3;
  const char **cmdArgv = argv + 3;
  unsigned int actualLen;
  struct AmlUsbRomRW rom = {};
  rom.pDataSize = &actualLen;
  if (argc <= 2) {
    dev_no = 0;
    cmdArgc = 0;
  } else {
#ifndef BUGFIX
    std::string strArgDev(argv[2]);
    if (strArgDev.compare(0, 3, "dev") == 0) {
      if (strArgDev.length() > 5) {
#else
    const char *strArgDev = argv[2];
    size_t strArgDevLen = strlen(strArgDev);
    if (strArgDevLen >= 3 && memcmp(strArgDev, "dev", 3) == 0) {
      if (strArgDevLen > 5) {
#endif
        aml_printf("[update]ERR(L%d):", 1067);
        aml_printf(
          "devPara(%s) err\n",
#ifndef BUGFIX
          strArgDev.c_str()
#else
          strArgDev
#endif
        );
        goto end;
      }

      dev_no = strtol(
#ifndef BUGFIX
        strArgDev.substr(3, -1).c_str(),
#else
        &strArgDev[3],
#endif
        NULL, 10);
      if (dev_no > 16) {
        aml_printf("[update]ERR(L%d):", 1073);
        aml_printf("dev_no(%d) too large\n", dev_no);
        goto end;
      }

      cmdArgv = argv + 3;
#ifndef BUGFIX
    } else if (strArgDev.compare(0, 5, "path-") == 0) {
      char *devPath = (char *) strArgDev.substr(5, -1).c_str();
#else
    } else if (strArgDevLen >= 5 && memcmp(strArgDev, "path-", 4) == 0) {
      char *devPath = (char *) &strArgDev[5];
#endif
      aml_printf("[update]devPath is [%s]\n", devPath);
      rom.device = AmlGetDeviceHandle("WorldCup Device", devPath);
    } else {
      dev_no = 0;
      cmdArgv = argv + 2;
      cmdArgc++;
    }
  }

  if (!rom.device &&
      update_scan((void **) &rom.device, 0, dev_no, &success, NULL) != 0) {
    aml_printf("[update]ERR(L%d):", 1095);
    aml_printf("can not find device\n");
    goto end;
  }

  if (!rom.device) {
    aml_printf("[update]ERR(L%d):", 1099);
    aml_printf("can not open dev[%d] device, maybe it not exist!\n", dev_no);
    goto end;
  }

  if (strcmp(cmd, "run") == 0 || strcmp(cmd, "rreg") == 0) {
    ret = update_sub_cmd_run_and_rreg(rom, cmd, cmdArgv, cmdArgc);
    goto end;
  }

  if (strcmp(cmd, "password") == 0) {
    ret = update_sub_cmd_set_password(rom, cmdArgv, cmdArgc);
    goto end;
  }

  if (strcmp(cmd, "chipinfo") == 0) {
    if (cmdArgc <= 0) {
      aml_printf("[update]ERR(L%d):", 1117);
      aml_printf("paraNum(%d) too small for chipinfo\n", argc);
      goto end;
    }

    ret = update_sub_cmd_get_chipinfo(rom, cmdArgv, cmdArgc);
    goto end;
  }

  if (strcmp(cmd, "chipid") == 0) {
    ret = update_sub_cmd_get_chipid(rom, cmdArgv);
    goto end;
  }

  if (strcmp(cmd, "write") == 0 || strcmp(cmd, "read") == 0 ||
      strcmp(cmd, "wreg") == 0 || strcmp(cmd, "dump") == 0 ||
      strcmp(cmd, "boot") == 0 || strcmp(cmd, "cwr") == 0 ||
      strcmp(cmd, "write2") == 0) {
    ret = update_sub_cmd_read_write(rom, cmd, cmdArgv, cmdArgc);
    goto end;
  }

  if (strcmp(cmd, "msdev") == 0 || strcmp(cmd, "msget") == 0 ||
      strcmp(cmd, "msset") == 0) {
    const char *s1;

    if (strcmp(cmd, "msset") == 0) {
      dev_no = 0;
      if (argc == 4) {
        if (strlen(argv[2]) != 4 || strncmp(argv[2], "dev", 3) != 0) {
          puts("please input dev_no like 'dev0'");
          goto end;
        }
        dev_no = argv[2][3] - '0';
      } else if (argc != 3) {
        goto help_end;
      }
      s1 = argv[argc - 1];
      if (argc == 3 && strncmp(s1, "dev", 3) == 0) {
        char yesno;
        printf("if you want input cmd \"%s\" (Y/N): ", s1);
        scanf("%c", &yesno);
        if (strncasecmp(&yesno, "y", 1) != 0) {
          goto end;
        }
      }
    } else if (argc == 2) {
      dev_no = 0;
    } else {
      if (argc != 3) {
        goto help;
      }
      if (strlen(argv[2]) != 4 || strncmp(argv[2], "dev", 3) != 0) {
        puts("please input dev_no like 'dev0'");
        goto end;
      }
      dev_no = argv[2][3] - '0';
    }

    if (update_scan(NULL, 0, dev_no, &success, scan_mass_storage) <= 0) {
      puts("can not find device");
      goto end;
    }

    printf(" %s\n", scan_mass_storage);
    if (strcmp(cmd, "msget") == 0) {
      char buf[128] = {};
      if (AmlGetUpdateComplete(NULL, buf, NULL) != 0) {
        puts("AmlGetUpdateComplete error");
      } else {
        printf("AmlGetUpdateComplete = %s  %lu\n", buf, 0ul);
      }
    }
    if (strcmp(cmd, "msset") == 0) {
      if (AmlSetFileCopyCompleteEx(NULL, (char *) s1, strlen(s1)) != 0) {
        printf("AmlSetFileCopyCompleteEx error--%s\n", s1);
      } else {
        printf("AmlSetFileCopyCompleteEx success--%s\n", s1);
      }
    }
    goto end;
  }

  if (strcmp(cmd, "identify") == 0) {
    update_sub_cmd_identify_host(rom, cmdArgc ? atoi(cmdArgv[0]) : 4, NULL);
    goto end;
  }

  if (strcmp(cmd, "reset") == 0) {
    if (AmlResetDev(&rom) != 0) {
      aml_printf("[update]ERR(L%d):", 1221);
      aml_printf("ERR: get info from device failed\n");
    } else {
      aml_printf("[update]reset succesful\n");
    }
    goto end;
  }

  if (strcmp(cmd, "tplcmd") == 0) {
    update_sub_cmd_tplcmd(rom, argv[argc - 1]);
    goto end;
  }

  if (strcmp(cmd, "burn") == 0) {
    if (AmlUsbburn(
        rom.device, "d:/u-boot.bin", 0x49000000, (char *) "nand", 0x12345678,
        0x2000, 0) < 0) {
      puts("ERR: get info from device failed");
    }
    goto end;
  }

  if (strcmp(cmd, "tplstat") == 0) {
    char buf[0x40] = {};
    rom.buffer = buf;
    rom.bufferLen = sizeof(buf);
    if (AmlUsbReadStatus(&rom) != 0) {
      puts("ERR: AmlUsbReadStatus failed!");
    } else {
      printf("reply %s \n", buf);
    }
    goto end;
  }

  if (strcmp(cmd, "skscan") == 0 || strcmp(cmd, "skgsn") == 0 ||
      strcmp(cmd, "skssn") == 0) {
    aml_scan_init();

    bool isSkssn = strcmp(cmd, "skssn") == 0;
    if (isSkssn + 2 == argc) {
      dev_no = 0;
    } else if (isSkssn + 3 == argc) {
      if (strncmp(argv[2], "dev", 3) != 0) {
        puts("please input dev_no like 'dev0'");
        goto end;
      }
      dev_no = argv[2][3] - '0';
    } else if (strcmp(cmd, "skscan") != 0) {
      goto help_end;
    }

    char *candidateDevices[8] = {};
    printf("发现 %d 个设备\n", aml_scan_usbdev(candidateDevices));
    if (strcmp(cmd, "skgsn") == 0) {
      char buf[512] = {};
      int len = aml_get_sn(candidateDevices[dev_no], buf);
      if (len == 0) {
        puts("此设备没有写过SN ");
      } else if (len == -1) {
        printf("没有此设备[设备号%d]\n", dev_no);
      } else if (len > 0) {
        buf[len] = 0;
        printf("此设备的SN :%s\n", buf);
      }
    } else if (strcmp(cmd, "skssn") == 0) {
      int len = aml_set_sn(candidateDevices[dev_no], (char *) argv[argc - 1]);
      if (len == 0) {
        puts("写此设备的SN失败!!!");
      } else if (len == -1) {
        printf("没有此设备[设备号%d]\n", dev_no);
      } else if (len > 0) {
        puts("写此设备的SN成功");
      }
    }
    aml_scan_close();
    goto end;
  }

  if (strcmp(cmd, "mwrite") == 0) {
    if (argc <= 5) {
      goto help_end;
    }

    ret = do_cmd_mwrtie(cmdArgv, cmdArgc, rom);
    goto end;
  }

  if (strcmp(cmd, "partition") == 0) {
    if (argc <= 3) {
      goto help_end;
    }

    int mwriteArgc = 4;
    const char *mwriteArgv[5] = {
      cmdArgv[1], "store", cmdArgv[0],
      cmdArgc > 2 ? cmdArgv[2] :
      is_file_format_sparse(cmdArgv[1]) ? "sparse" : "normal"};
    if (cmdArgc > 3) {
      mwriteArgv[4] = cmdArgv[3];
      mwriteArgc++;
    }
    ret = do_cmd_mwrtie(mwriteArgv, mwriteArgc, rom);
    goto end;
  }

  if (strcmp(cmd, "bulkcmd") == 0) {
    char buf[68] = {};
    memcpy(buf, argv[argc - 1], aml_min(strlen(argv[argc - 1]), 66));
    buf[66] = 1;

    rom.buffer = buf;
    rom.bufferLen = sizeof(buf);
    if (AmlUsbBulkCmd(&rom) != 0) {
      puts("ERR: AmlUsbBulkCmd failed!");
      ret = -1368;
      goto end;
    }

    ret = 0;
    goto end;
  }

  if (strcmp(cmd, "down") == 0) {
    if (argc == 4) {
      if (strlen(argv[2]) != 4 || strncmp(argv[2], "dev", 3) != 0) {
        puts("please input dev_no like 'dev0'");
        goto end;
      }
      dev_no = argv[2][3] - '0';
    } else {
      if (argc != 3) {
        goto help_end;
      }
      dev_no = 0;
    }

    if (update_scan((void **) &rom.device, 0, 0, &success, NULL) <= 0) {
      puts("can not find device");
    } else if (rom.device != NULL) {
      if (WriteMediaFile(&rom, argv[3]) != 0) {
        aml_printf("ERR:write data to media failed\n");
      }
    } else {
      printf("ERR: can not open dev%d device, maybe it not exist!\n", dev_no);
    }
    goto end;
  }

  if (strcmp(cmd, "mread") == 0) {
    ret = update_sub_cmd_mread(rom, cmdArgc, cmdArgv);
    goto end;
  }

  if (strcmp(cmd, "bl2_test") == 0) {
    ret = Aml_Libusb_bl2_test(rom.device, 5000, cmdArgc, cmdArgv);
    goto end;
  }

  if (strcmp(cmd, "bl2_boot") == 0) {
    ret = Aml_Libusb_bl2_boot(rom.device, 5000, cmdArgc, cmdArgv);
    goto end;
  }

help_end:
  update_help();

end:
  if (stream != NULL) {
    fclose(stream);
  }
  if (buf != NULL) {
    free(buf);
  }
  if (rom.device != NULL) {
    rom.device = NULL;
  }
  aml_uninit();
  return ret;
}
