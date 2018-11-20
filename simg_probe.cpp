#include <cstdio>
#include <cstring>

#include "Amldbglog/Amldbglog.h"


bool simg_probe (const unsigned char *buf, unsigned int len) {
  const unsigned char magic[] = {0x3A, 0xFF, 0x26, 0xED};
  const unsigned char magic2[] = {0x28, 0, 0x12, 0};

  if (len < 0x1C) {
    return false;
  }
  if (strncmp((const char *) buf, (const char *) magic, 4) != 0) {
    return false;
  }
  if (*((short *) buf + 2) != 1 ||
      strncmp((const char *) buf + 8, (const char *) magic2, 4) != 0) {
    return false;
  }
  aml_printf("[update]sparse format detected\n");
  return true;
}


bool is_file_format_sparse (const char *filename) {
  FILE *fp = fopen(filename, "rb");

  if (fp == nullptr) {
    aml_printf("[update]ERR(L%d):", 68);
    aml_printf("Fail to open file in mode rb\n");
    return false;
  }

  auto buf = new unsigned char[0x2000];
  size_t len = fread(buf, 1, 0x2000, fp);
  fclose(fp);
  bool result = simg_probe(buf, (unsigned int) len);
  if (buf) {
    delete[] buf;
  }
  return result;
}
