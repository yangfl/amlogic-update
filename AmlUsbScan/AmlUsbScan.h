#ifndef AML_USB_SCAN_H
#define AML_USB_SCAN_H


bool simg_probe (const unsigned char *buf, unsigned int len);
bool is_file_format_sparse (const char *filename);
int aml_scan_init ();
int aml_scan_close ();
int aml_scan_usbdev (char **candidate_devices);
int aml_send_command (void *device, const char *mem_type, int retry, char *reply);
int aml_get_sn (char *target_device, char *usid);
int aml_set_sn (char *target_device, char *usid);


#endif //AML_USB_SCAN_H
