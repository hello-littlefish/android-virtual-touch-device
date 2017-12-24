#ifndef VIRTUAL_MULTI_TOUCH_DEVICE_H_123AEDAD_ABBE_45BD_B2A3_DA3BFB95FD22
#define VIRTUAL_MULTI_TOUCH_DEVICE_H_123AEDAD_ABBE_45BD_B2A3_DA3BFB95FD22

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus

// Create a virtual multi-touch devic,that successfully returns the uinput
// file descriptor and fails to return -1.
int virtual_multi_touch_device_create(const char *name);

// Destroy virtual multi-touch device, return 0 successfully, fail to return -1.
int virtual_multi_touch_device_destroy(int fd);

// Write event to virtual multi-touch device, return 0 successfully, fail
// to return -1.
int virtual_multi_touch_device_write_event(int fd, unsigned short type, unsigned short code, unsigned int value);

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus

#endif // #ifndef VIRTUAL_MULTI_TOUCH_DEVICE_H_123AEDAD_ABBE_45BD_B2A3_DA3BFB95FD22
