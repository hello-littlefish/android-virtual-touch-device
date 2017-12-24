#include "virtual_multi_touch_device.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/limits.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define SIZEOF_BIT_ARRAY(bits)  (((bits) + 7) / 8)
#define TEST_BIT(bit, array)    ((array[(bit) / 8]) & (1 << ((bit) % 8)))

// Get multi-touch device absinfo, return 0 successfully, fail to return -1.
static int get_multi_touch_device_absinfo(struct input_absinfo *x, struct input_absinfo *y);

// Check device bitmask if it is a multi-touch device that returns 0, not -1.
static int is_multi_touch_device(const char *path);

// Create a virtual multi-touch devic,that successfully returns the uinput
// file descriptor and fails to return -1.
int virtual_multi_touch_device_create(const char *name) {
  // Get multi-touch device absinfo
  struct input_absinfo x = { 0 };
  struct input_absinfo y = { 0 };
  if (get_multi_touch_device_absinfo(&x, &y) == -1) {
    return -1;
  }

  // Open uinput
  int uinput_fd = - 1;
  if ((uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) == -1 &&
      (uinput_fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK)) == -1 &&
      (uinput_fd = open("/dev/misc/uinput", O_WRONLY | O_NONBLOCK)) == -1 &&
      (uinput_fd = open("/android/dev/uinput", O_WRONLY | O_NONBLOCK)) == -1) {
    return -1;
  }

  // Configure virtual multi-touch device event properties
  struct uinput_user_dev uinp = { 0 };
  strncpy(uinp.name, name, sizeof (uinp.name));
  uinp.id.version = 4;
  uinp.id.bustype = BUS_USB;
  uinp.absmin[ABS_MT_SLOT] = 0;
  uinp.absmax[ABS_MT_SLOT] = 9;
  uinp.absmin[ABS_MT_TOUCH_MAJOR] = 0;
  uinp.absmax[ABS_MT_TOUCH_MAJOR] = 15;
  uinp.absmin[ABS_MT_POSITION_X] = x.minimum;
  uinp.absmax[ABS_MT_POSITION_X] = x.maximum;
  uinp.absmin[ABS_MT_POSITION_Y] = y.minimum;
  uinp.absmax[ABS_MT_POSITION_Y] = y.maximum;
  uinp.absmin[ABS_MT_TRACKING_ID] = 0;
  uinp.absmax[ABS_MT_TRACKING_ID] = 65535;
  uinp.absmin[ABS_MT_PRESSURE] = 0;
  uinp.absmax[ABS_MT_PRESSURE] = 255;
  if (write(uinput_fd, &uinp, sizeof (uinp)) == -1) {
    close(uinput_fd);
    uinput_fd = -1;
    return -1;
  }

  // Set virtual multi-touch device bitmask
  if (ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_SLOT) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_POSITION_X) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID) == -1 ||
      ioctl(uinput_fd, UI_SET_ABSBIT, ABS_MT_PRESSURE) == -1 ||
      ioctl(uinput_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT) == -1) {
    close(uinput_fd);
    uinput_fd = -1;
    return -1;
  }

  // Create virtual multi-touch devic
  return (ioctl(uinput_fd, UI_DEV_CREATE) == -1 ? -1 : uinput_fd);
}

// Destroy virtual multi-touch device, return 0 successfully, fail to return -1.
int virtual_multi_touch_device_destroy(int fd) {
  if (ioctl(fd, UI_DEV_DESTROY) == -1) {
    return -1;
  }
  return close(fd);
}

// Write event to virtual multi-touch device, return 0 successfully, fail
// to return -1.
int virtual_multi_touch_device_write_event(int fd, unsigned short type, unsigned short code, unsigned int value) {
  // Fill event
  struct input_event event = { 0 };
  if (gettimeofday(&event.time, NULL) == -1) {
    return -1;
  }
  event.type = type;
  event.code = code;
  event.value = value;

  // Write event
  return (write(fd, &event, sizeof(event)) == -1 ? -1 : 0);
}

// Get multi-touch device absinfo, return 0 successfully, fail to return -1.
int get_multi_touch_device_absinfo(struct input_absinfo *x, struct input_absinfo *y) {
  // Open the directory
  DIR *dir = opendir("/dev/input");
  if (dir == NULL) {
    return -1;
  }

  // Traverse the device
  struct dirent *de = NULL;
  while ((de = readdir(dir)) != NULL) {
    // Filter the file name
    if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
        (de->d_name[0] == '.' && de->d_name[1] == '.' && de->d_name[2] == '\0')) {
      continue;
    }

    // Format the device path
    char device_path[PATH_MAX] = { 0 };
    sprintf(device_path, "%s%s", "/dev/input/", de->d_name);

    // Check if it is a multi-touch device
    if (is_multi_touch_device(device_path) == -1) {
      continue;
    }

    // Get absinfo
    int device_fd = open(device_path, O_RDONLY);
    if (device_fd == -1) {
      continue;
    }

    if (ioctl(device_fd, EVIOCGABS(ABS_MT_POSITION_X), x) == -1 ||
        ioctl(device_fd, EVIOCGABS(ABS_MT_POSITION_Y), y) == -1) {
      close(device_fd);
      device_fd = -1;
      closedir(dir);
      dir = NULL;
      return -1;
    }

    return 0;
  } // while ((de = readdir(dir)) != NULL) {

  // Close the directory
  closedir(dir);
  dir = NULL;
  return -1;
}

// Check device bitmask if it is a multi-touch device that returns 0, not -1.
int is_multi_touch_device(const char *path) {
  // Open device
  int device_fd = open(path, O_RDONLY);
  if (device_fd == -1) {
    return -1;
  }

  // Get device bitmask
  unsigned char abs_bitmask[SIZEOF_BIT_ARRAY(ABS_MAX + 1)];
  if (ioctl(device_fd, EVIOCGBIT(EV_ABS, sizeof (abs_bitmask)), abs_bitmask) == -1) {
    close(device_fd);
    device_fd = -1;
    return -1;
  }

  // Close device
  close(device_fd);
  device_fd = -1;

  // Check device bitmask
  return (TEST_BIT(ABS_MT_POSITION_X, abs_bitmask) != 0 && 
          TEST_BIT(ABS_MT_POSITION_Y, abs_bitmask) != 0 ? 0 : -1);
}
