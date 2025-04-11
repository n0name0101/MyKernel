#include <filesystem/devtmpsys.h>
#include <filesystem/vfilesystem.h>
#include <string.h>
#include <utils.h>

dev_file_t dev_table[MAX_DEVICES];
uint32_t dev_table_len;

filesystem_t DeviceFileManager = {
   .open = NULL
};

// Declaration
file_t* devtmpfs_filesystem_open(const char* rel_path, int flags);

void initDevTmpSys() {
   memset(dev_table, 0, sizeof(dev_table));
   dev_table_len = 0;

   DeviceFileManager = (filesystem_t) {
      .open = devtmpfs_filesystem_open
   };
}

int32_t register_device(const char* device_name, file_ops_t* file_operation) {
   if (dev_table_len >= MAX_DEVICES) return -1;

   strncpy(dev_table[dev_table_len].device_name, device_name, sizeof(dev_table[dev_table_len].device_name)-1);
   dev_table[dev_table_len].device_name[sizeof(dev_table[dev_table_len].device_name)-1] = '\0';
   dev_table[dev_table_len].file_operation = file_operation;
   dev_table_len++;
   return 0;
}


file_t* devtmpfs_filesystem_open(const char* rel_path, int flags) {
   for (uint32_t i = 0; i < dev_table_len; ++i) {
      if (strcmp(dev_table[i].device_name, rel_path) == 0) {
         static file_t testing = {0};
         (&testing)->file_operation  = dev_table[i].file_operation;
         (&testing)->vFileSystemData = NULL;
         (&testing)->error_code = 0;
         return &testing;
         // file_t* f = malloc(sizeof(file_t));
         // if (!f) return NULL;
         //
         // f->ops = dev_table[i].fops;
         // f->priv = dev_table[i].priv_data;
         // f->pos = 0;
         // error_code
         //
         // return f;
      }
   }

   // Device tidak ditemukan
   return NULL;
}
