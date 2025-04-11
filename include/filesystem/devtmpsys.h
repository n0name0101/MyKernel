#ifndef __DEVTMPSYS_H
#define __DEVTMPSYS_H

#include <types.h>
#include <filesystem/vfilesystem.h>

typedef struct dev_file {
    char device_name[64];
    file_ops_t* file_operation;  // fungsi read/write/open/close device
} dev_file_t;

#define MAX_DEVICES 64

// Function Declaration
void initDevTmpSys();
int32_t register_device(const char* device_name, file_ops_t* file_operation);

#endif               //__DEVTMPSYS_H
