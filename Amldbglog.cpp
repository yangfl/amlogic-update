#include <stdio.h>
#include <stdarg.h>

#include "Amldbglog.h"


static FILE *stream;


int aml_printf (const char *format, ...) {
  int ret;
  va_list args;

  va_start(args, format);
  if (stream != NULL) {
    ret = vfprintf(stream, format, args);
  } else {
    ret = vprintf(format, args);
  }
  va_end(args);

  return ret;
}


int aml_open_logfile (const char *filename) {
  if (filename) {
    stream = fopen(filename, "a+");
  }
  return stream ? 0 : -1;
}


void aml_close_logfile (void) {
  if (stream == NULL) {
    return;
  }

  fclose(stream);
  stream = NULL;
}


int aml_init (void) {
  return 0;
}


void aml_uninit (void) {
  return;
}
