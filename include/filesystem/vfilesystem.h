#ifndef __VFILESYSTEM_H
#define __VFILESYSTEM_H

#include <types.h>

#define MAX_MOUNT_FILESYSTEM 20

#define VFS_NO_ERROR         0
#define VFS_INVALID_OP      -1
#define VFS_ERR_NOT_FOUND   -2

typedef struct file file_t;

typedef struct file_ops {
   int32_t (*read)(file_t, char* buf, uint32_t size);
   int32_t (*write)(file_t, const char* buf, uint32_t size);
   int32_t (*readdir)(file_t, char* name_buf);
   int32_t (*close)(file_t);
} file_ops_t;

// Contoh definisi struktur file dan filesystem
typedef struct file {
    char name[128];
    file_ops_t *file_operation;
    void *vFileSystemData;
    int32_t error_code;
    /* data file */
} file_t;

typedef struct filesystem {
    // Fungsi open: menerima relative path dan flags, kemudian mengembalikan pointer file
    file_t* (*open)(const char* rel_path, int32_t flags);
} filesystem_t;

// Struktur mount untuk menyimpan mount point dan filesystem terkait
typedef struct mount {
    char mount_point[128];   // Contoh: "/" atau "/dev"
    filesystem_t* filesystem;        // Pointer ke struktur filesystem
} mount_t;

// Declaration
void initVFileSystem();
file_t* vfs_open(const char* path, int flags);
int32_t vfs_read(file_t* file, char* buf, uint32_t size);
int32_t vfs_write(file_t* file, const char* buf, uint32_t size);
int32_t vfs_mount(const char* mount_point, filesystem_t* fs);

#endif               //__VFILESYSTEM_H
