#ifndef AML_UPDATE_H
#define AML_UPDATE_H

#include <string>

#include "UsbRomDrv/UsbRomDll.hpp"


#define AML_CHIP_ID_LEN 12


struct DownloadProgressInfo {
  long nBytes;  // in bytes
  int percentage_0;
  int percentage_100;
  unsigned int percentage_100_remain;
  int percentage;
  long nBytesDownloaded;
  int startTime;
  int total_div_100;
  unsigned int max_4k_total_div_100;
  int max_1_4k_div_total_mul_100;
  char prompt[16];

  DownloadProgressInfo (long long total_, const char *prompt_);
  int update_progress (unsigned int dataLen);
};


std::string strToLower (const std::string &src);
unsigned long simple_strtoul (const char *cp, char **endp, unsigned int base);
int _print_memory_view (char *buf, unsigned int size, unsigned int offset);
int update_help ();
int update_scan (void **resultDevices, int print_dev_list, unsigned int dev_no, int *success, char *scan_mass_storage);
int do_cmd_mwrtie (const char **argv, signed int argc, AmlUsbRomRW &rom);
int update_sub_cmd_run_and_rreg (AmlUsbRomRW &rom, const char *cmd, const char **argv, signed int argc);
int update_sub_cmd_set_password (AmlUsbRomRW &rom, const char **argv, int argc);
int update_sub_cmd_get_chipinfo (AmlUsbRomRW &rom, const char **argv, signed int argc);
int update_sub_cmd_read_write (AmlUsbRomRW &rom, const char *cmd, const char **argv, int argc);
int update_sub_cmd_identify_host (AmlUsbRomRW &rom, int idLen, char *id);
int update_sub_cmd_get_chipid (AmlUsbRomRW &rom, const char **argv);
int update_sub_cmd_tplcmd (AmlUsbRomRW &rom, const char *tplCmd);
int update_sub_cmd_mread (AmlUsbRomRW &rom, int argc, const char **argv);
int main (int argc, const char **argv);
int WriteMediaFile(AmlUsbRomRW *rom, const char *filename);
int ReadMediaFile (AmlUsbRomRW *rom, const char *filename, long size);


#endif  // AML_UPDATE_H
