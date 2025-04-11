// Struktur direktori: // /driver/ata.c           -> ATA driver // /fs/block_device.c      -> Abstraksi block device // /fs/simplefs.c          -> Implementasi FS sederhana // /vfs/vfs.c              -> Virtual FileSystem layer // /shell/shell.c          -> Shell minimalis // /kernel/main.c          -> Entry point kernel

// ============================= // /driver/ata.c // =============================

int ata_read_sector(uint64_t lba, void* buf);
int ata_write_sector(uint64_t lba, const void* buf);

// ============================= // /fs/simplefs.c // =============================
#include "../fs/block_device.h"
#include "../vfs/vfs.h"
#include <string.h>

int simplefs_read(file_t* f, void* buf, size_t size);
int simplefs_readdir(file_t* f, char* name_buf);
int simplefs_close(file_t* f);

int simplefs_mount(block_device_t* dev) {
   return 0;
}

file_t* simplefs_open(const char* path) {
   static struct file dummy_file;
   static struct file_ops dummy_ops = {
      .read = simplefs_read,
      .readdir = simplefs_readdir,
      .close = simplefs_close
   };
   dummy_file.ops = &dummy_ops;
   dummy_file.fs_data = 0;
   return &dummy_file;
}

int simplefs_read(file_t* f, void* buf, size_t size) {
   const char* dummy_content = "Hello from FS!\n";
   size_t len = strlen(dummy_content);
   memcpy(buf, dummy_content, len);
   return len;
}

int simplefs_readdir(file_t* f, char* name_buf) {
   static int called = 0;
   if (called++) return 0;
   strcpy(name_buf, "file.txt");
   return 1;
}

int simplefs_close(file_t* f) {
   return 0;
}

struct filesystem simple_fs = {
   .mount = simplefs_mount,
   .open = simplefs_open
};

// ============================= // /fs/block_device.c // =============================

#include "block_device.h"

block_device_t* global_block_device = 0;

int block_read(uint64_t lba, uint32_t count, void* buffer) {
   return global_block_device->read(lba, count, buffer);
}

int block_write(uint64_t lba, uint32_t count, const void* buffer) {
   return global_block_device->write(lba, count, buffer);
}

void register_block_device(block_device_t* dev) {
   global_block_device = dev;
}

// ============================= // /vfs/vfs.c // ============================= #include "vfs.h" #include <string.h>

static struct filesystem* active_fs = 0;
static char current_working_dir[128] = "/";

int vfs_mount(struct filesystem* fs, block_device_t* dev) {
   active_fs = fs;
   return fs->mount(dev);
}

void vfs_set_cwd(const char* path) {
   strncpy(current_working_dir, path, sizeof(current_working_dir));
   current_working_dir[sizeof(current_working_dir)-1] = '\0';
}

const char* vfs_get_cwd() {
   return current_working_dir;
}

file_t* vfs_open(const char* path) {
   static char full_path[256];
   if (path[0] != '/') {
      snprintf(full_path, sizeof(full_path), "%s/%s", current_working_dir, path);
   }
   else {
      strncpy(full_path, path, sizeof(full_path));
   }
   return active_fs->open(full_path);
}

int vfs_read(file_t* f, void* buf, size_t size) {
   return f->ops->read(f, buf, size);
}

int vfs_readdir(file_t* f, char* name_buf) {
   return f->ops->readdir(f, name_buf);
}

int vfs_close(file_t* f) {
   return f->ops->close(f);
}

// ============================= // /vfs/vfs.h // ============================= #ifndef VFS_H #define VFS_H #include <stddef.h> #include <stdint.h> #include "../fs/block_device.h"

typedef struct file file_t;

struct file_ops {
   int (read)(file_t, void* buf, size_t size);
   int (readdir)(file_t, char* name_buf);
   int (close)(file_t);
};

struct file {
   struct file_ops* ops;
   void* fs_data;
};

struct filesystem {
   int (mount)(block_device_t);
   file_t* (open)(const char path);
};

int vfs_mount(struct filesystem* fs, block_device_t* dev);
file_t* vfs_open(const char* path);
int vfs_read(file_t* f, void* buf, size_t size);
int vfs_readdir(file_t* f, char* name_buf);
int vfs_close(file_t* f);
void vfs_set_cwd(const char* path);
const char* vfs_get_cwd();

#endif

// ============================= // /shell/shell.c // ============================= #include <stdio.h> #include <string.h> #include "../vfs/vfs.h"

void shell_loop() {
   char input[128];
   while (1) {

      printf("%s> ", vfs_get_cwd()); gets(input);

      if (strncmp(input, "cat ", 4) == 0) {
           file_t* f = vfs_open(input + 4);
           char buf[128];
           int n = vfs_read(f, buf, sizeof(buf));
           buf[n] = '\0';
           printf("%s", buf);
           vfs_close(f);
      } else if (strcmp(input, "ls") == 0) {
         file_t* f = vfs_open(".");
         char name[64];
         while (vfs_readdir(f, name)) {
            printf("%s\n", name);
         }
         vfs_close(f);
      }
      else if (strncmp(input, "cd ", 3) == 0)
         vfs_set_cwd(input + 3);
      else
         printf("Unknown command\n");
   }

}

// ============================= // /kernel/main.c // ============================= #include "../fs/block_device.h" #include "../fs/simplefs.c" #include "../vfs/vfs.h" #include "../shell/shell.c"

extern int ata_read_sector(uint64_t lba, void* buf); extern int ata_write_sector(const void* buf);

void kernel_main() {
   static block_device_t ata_dev = { .read = ata_read_sector, .write = ata_write_sector, .total_sectors = 1024 * 1024 };

   register_block_device(&ata_dev);
   vfs_mount(&simple_fs, &ata_dev);
   vfs_set_cwd("/");
   shell_loop();
}
