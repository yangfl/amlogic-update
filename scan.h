#ifndef AML_USB_SCAN_H
#define AML_USB_SCAN_H


int aml_scan_init (void);
int aml_scan_close (void);
int aml_scan_usbdev (char **candidateDevices);
int aml_send_command (void *dev, char *cmd, int retry, char *reply);
int aml_get_sn (char *targetDevice, char *usid);
int aml_set_sn (char *targetDevice, char *usid);


#endif  // AML_USB_SCAN_H
