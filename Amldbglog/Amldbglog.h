#ifndef AML_DBG_LOG_H
#define AML_DBG_LOG_H


int aml_printf (const char *format, ...);
int aml_open_logfile (const char *filename);
int aml_close_logfile (void);
int aml_init (void);
int aml_uninit (void);


#endif  // AML_DBG_LOG_H
