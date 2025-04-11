#include <filesystem/vfilesystem.h>
#include <utils.h>
#include <string.h>

mount_t mount_list[MAX_MOUNT_FILESYSTEM];                      // Mount Device or File Filesystem
uint32_t mount_list_len;

void initVFileSystem() {
   // Memset Mount List;
   memset(mount_list, 0, sizeof(mount_list));
   mount_list_len = 0;
}

// Fungsi untuk men-*mount* filesystem ke mount point tertentu
int32_t vfs_mount(const char* mount_point, filesystem_t* fs) {
   if (mount_list_len >= MAX_MOUNT_FILESYSTEM) return -1;    // Mounting Failed

   strncpy(mount_list[mount_list_len].mount_point, mount_point, sizeof(mount_list[mount_list_len].mount_point)-1);
   mount_list[mount_list_len].mount_point[sizeof(mount_list[mount_list_len].mount_point)-1] = '\0';
   mount_list[mount_list_len].filesystem = fs;
   mount_list_len++;

   return 0;
}

// Fungsi vfs_open() untuk mencari mount point yang tepat berdasarkan path
file_t* vfs_open(const char* path, int flags) {
   mount_t* best_mount = NULL;
   uint32_t best_len = 0;

   // Cari mount point yang merupakan prefix dari path
   for (uint32_t i = 0; i < mount_list_len ; i++) {
      mount_t* m = &mount_list[i];
      uint32_t len = strlen(m->mount_point);
      // Pastikan m->mount_point di awal path dan
      // pilih yang terpanjang (paling spesifik)
      if (strncmp(path, m->mount_point, len) == 0 && len > best_len) {
         best_mount = m;
         best_len = len;
      }
   }

   if (best_mount == NULL) return NULL;      // Path not Found

   // Hitung relative path: Hilangkan prefix mount point dari path
   const char* rel_path = path + best_len;
   if (*rel_path == '\0') {
     rel_path = "/";  // Bila path adalah mount point itu sendiri
   }

   // Panggil fungsi open dari filesystem yang dipilih
   return best_mount->filesystem->open(rel_path, flags);
}

int32_t vfs_read(file_t* file, char* buf, uint32_t size) {
    if (!file || !file->file_operation->read) {
        if (file) file->error_code = -1; // Misalnya -1 untuk invalid op
        return -1;
    }
    int32_t result = file->file_operation->read(*file, buf, size);
    file->error_code = 0; // Clear error code
    if (result < 0) {
        file->error_code = result; // Simpan error
    }
    return result;
}

int32_t vfs_write(file_t* file, const char* buf, uint32_t size) {
    if (!file || !file->file_operation->write) {
        if (file) file->error_code = -1; // Misalnya: Invalid operation
        return -1;
    }

    int32_t result = file->file_operation->write(*file, buf, size);
    file->error_code = 0; // Clear error code
    if (result < 0) {
        file->error_code = result; // Simpan error jika gagal
    }
    return result;
}

int32_t vfs_ferror(file_t* file) {
    if (!file) return -1;
    return file->error_code;
}

void vfs_clearerr(file_t* file) {
    if (file) file->error_code = 0;
}

const char* vfs_strerror(int32_t err) {
    switch (err) {
        case VFS_NO_ERROR        : return "No error";
        case VFS_INVALID_OP      : return "Invalid operation";
        case VFS_ERR_NOT_FOUND   : return "File not found";
        default: return "Unknown error";
    }
}
