#include <stdio.h>
#include <stdarg.h>

#include "Amldbglog.h"


static FILE *fp;


int aml_printf (const char *format, ...) {
  int result;
  va_list args;

  va_start(args, format);
  if (fp) {
    result = vfprintf(fp, format, args);
  } else {
    result = vprintf(format, args);
  }
  va_end(args);

  return result;
}


int aml_open_logfile (const char *filename) {
  if (filename) {
    fp = fopen(filename, "a+");
  }
  return fp ? 0 : -1;
}

int aml_close_logfile (void) {
  if (fp == 0) {
    return 0;
  }

  int result = fclose(fp);

  fp = 0;
  return result;
}


int aml_init (void) {
  return 0;
}


int aml_uninit (void) {
  return 0;
}
