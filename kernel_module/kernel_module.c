#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <asm/pgtable.h>
#include <linux/fd.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/vfs.h>
#include <linux/mutex.h>
#include <linux/namei.h>


#define BUFFER_SIZE 128
static DEFINE_MUTEX(kernel_module_mutex);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("artemiyjjj");
MODULE_VERSION("1.0");

static struct dentry *kernel_module_root;
static struct dentry *kernel_module_args_file;
static struct dentry *kernel_module_result_file;

// static struct nameidata* nd = {0};
static struct path* path = {0};
static struct super_block* sb;
static struct kstatfs* stats;
static char arg_filename[BUFFER_SIZE];
static char kernel_buffer[BUFFER_SIZE];

static int get_path_by_name(char* path_name);
static int get_sb_by_path(void);
static int get_stats_by_denty(void);

static int print_data(struct seq_file *sf, void *data) {
    printk(KERN_INFO "Writing to file.\n");
    if (NULL == path) {
        seq_printf(sf, "Found no file or directory by given path.\n");
        return 0;
    } else if (NULL == sb) {
        seq_printf(sf, "Given path does not lead to mount filesystem.\n");
        return 0;
    } else if (NULL == stats) {
        seq_printf(sf, "Statistics on filesystem are unavaliabe.\n");
        return 0;
    } else {
        seq_printf (sf, "======================================");
        seq_printf (sf, "File System statistics:");
        seq_printf (sf, "File system type: %s", sb -> s_type -> name);
        seq_printf (sf, "Mount on: %s", sb -> s_id);
        seq_printf (sf, "Block size: %lu", sb -> s_blocksize);
        seq_printf (sf, "Max file size: %llu bytes", sb -> s_maxbytes);
        seq_printf (sf, "\tTotal blocks: %llu", stats -> f_blocks);
        seq_printf (sf, "\tFree blocks: %llu", stats -> f_bfree);
        // seq_printf (sf, "\tUsed blocks: %llu", stats -> f_files);
        seq_printf (sf, "\tTotal inodes: %llu", stats -> f_files);
        seq_printf (sf, "\tFree inodes: %llu", stats -> f_ffree);
    }
    printk(KERN_INFO "Data successfully have been written.\n");
    return 1;
}

static int kmod_open(struct inode *inode, struct file *file) {
    // if (!mutex_trylock(&kernel_module_mutex)) {
    //     printk(KERN_INFO "can't lock file");
    //     return -EBUSY;
    // }
    mutex_lock(&kernel_module_mutex);
    printk(KERN_INFO "file is locked by module");
    return single_open(file, print_data, NULL);
}



static ssize_t kmod_args_write( struct file* file, const char __user* buffer, size_t length, loff_t* ptr_offset ) {
    // char user_data[BUFFER_SIZE];
    // char fs_path[length];
    printk(KERN_INFO "df module: getting argumments, size = %llu ...\n", length);
    printk(KERN_INFO "User buffer contents: %s", buffer);

    if (copy_from_user(kernel_buffer, buffer, length)) {
        printk(KERN_ERR "df module: failed writing to kernel\n");
        return -EFAULT;
    }
    printk(KERN_INFO "df module: user input successfully read.\n");
    if (sscanf(kernel_buffer, "path: %s", arg_filename) == 1) {
        printk(KERN_INFO "df module: path from argument: %s\n", arg_filename);
    } else {
        printk(KERN_ERR "df module: argument is not formatted correctly. Check README file.\n");
        return -EINVAL;
    }
    get_path_by_name(arg_filename);
    if (!get_sb_by_path()) {
        printk(KERN_ERR "Given path is not mount root.\n");
        return -EFAULT;
    }
    get_stats_by_denty();
    single_open(file, print_data, NULL);
    printk(KERN_INFO "Structures are filled successfully.\n");
    return strlen(kernel_buffer);
}

static int kmod_release(struct inode *inode, struct file *file) {
    mutex_unlock(&kernel_module_mutex);
    printk(KERN_INFO "file is unlocked by module");
    return 0;
}

static struct file_operations kernel_module_args_ops = {
        .owner   = THIS_MODULE,
        .read    = seq_read,
        .write   = kmod_args_write,
        .open    = kmod_open,
        .release = kmod_release
};

static int get_path_by_name(char* name) {
    int res = kern_path(name, LOOKUP_FOLLOW, path);
    if (res) {
        printk(KERN_ERR "df module: Failed to lookup %s.\n", name);
        return 1;
    }
    return 0;
}

static int get_sb_by_path(void) {
    if (path == NULL || !path_is_mountpoint(path)) {
        return 1;
    }
    sb = path -> dentry -> d_sb;
    return 0;
}

static int get_stats_by_denty(void) {
    return vfs_statfs(path, stats);
}


static int __init kernel_module_init(void) {
    printk(KERN_INFO "Kernel df reader: module loaded.\n");
    kernel_module_root = debugfs_create_dir("df_stat", NULL);
    kernel_module_args_file = debugfs_create_file("kernel_module_args", 0777, kernel_module_root, NULL, &kernel_module_args_ops);
    kernel_module_result_file = debugfs_create_file("kernel_module_result", 0777, kernel_module_root, NULL, &kernel_module_args_ops);
    return 0;
}

static void __exit kernel_module_exit(void) {
    mutex_destroy(&kernel_module_mutex);
    debugfs_remove_recursive(kernel_module_root);
    printk(KERN_INFO "Kernel df reader: module unloaded.\n");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);