#ifndef AML_DBG_LOG_H
#define AML_DBG_LOG_H

#include "defs.h"


__attr_format((printf, 1, 2))
int aml_printf (const char *format, ...);
int aml_open_logfile (const char *filename);
void aml_close_logfile (void);
int aml_init (void);
void aml_uninit (void);


#endif  // AML_DBG_LOG_H
