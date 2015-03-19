/*
 * vfs/vfs.h - Virtual File System interface
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <system.h>
#include <errno.h>
#include <types.h>
#include <atomic.h>
#include <rwlock.h>
#include <list.h>
#include <vfs-errno.h>
#include <vfs-params.h>
#include <vfs-private.h>
#include <ku_transfert.h>
#include <metafs.h>

struct vfs_node_s;
struct vfs_node_op_s;
struct vfs_file_op_s;
struct vfs_context_op_s;
struct vm_region_s;

struct vfs_dirent_s
{
	uint32_t d_ino;
	uint32_t d_type;
	uint8_t  d_name[VFS_MAX_NAME_LENGTH];
};

struct vfs_stat_s 
{
	uint_t    st_dev;     /* ID of device containing file */
	uint_t    st_ino;     /* inode number */
	uint_t    st_mode;    /* protection */
	uint_t    st_nlink;   /* number of hard links */
	uint_t    st_uid;     /* user ID of owner */
	uint_t    st_gid;     /* group ID of owner */
	uint_t    st_rdev;    /* device ID (if special file) */
	uint64_t  st_size;    /* total size, in bytes */
	uint_t    st_blksize; /* blocksize for file system I/O */
	uint_t    st_blocks;  /* number of 512B blocks allocated */
	time_t    st_atime;   /* time of last access */
	time_t    st_mtime;   /* time of last modification */
	time_t    st_ctime;   /* time of last status change */
};

struct vfs_context_s
{
	uint_t  ctx_type;
	struct device_s *ctx_dev;
	struct vfs_context_op_s *ctx_op;
	struct vfs_node_op_s *ctx_node_op;
	struct vfs_file_op_s *ctx_file_op;
	void *ctx_pv;
};

struct vfs_node_s
{
	char     n_name[VFS_MAX_NAME_LENGTH];
	uint_t   n_count;
	uint_t   n_flags;
	uint_t   n_attr;
	uint_t   n_mode;
	uint_t   n_state;
	uint64_t n_size;
	uint_t   n_links;
	uint_t   n_type;
	uint_t   n_uid;
	uint_t   n_gid;
	uint_t   n_acl;
	struct metafs_s n_meta;
	struct rwlock_s n_rwlock;
	struct wait_queue_s   n_wait_queue;
	struct vfs_node_op_s *n_op;
	struct vfs_context_s *n_ctx;
	struct vfs_node_s *n_parent;
	struct mapper_s   *n_mapper;
	struct vfs_node_s *n_mounted_point;
	struct list_entry  n_freelist;
	struct vfs_stat_s  n_stat;
	void    *n_pv;
};

struct vfs_file_s
{
	atomic_t f_count;
	uint_t f_offset;
	uint_t f_flags;
	uint_t f_mode;
	uint_t f_version;
	struct rwlock_s f_rwlock;
	struct vfs_node_s *f_node;
	struct vfs_file_op_s *f_op;
	struct list_entry f_wait_queue;
	void *f_pv;
};


#define VFS_CREATE_CONTEXT(n)   error_t (n) (struct vfs_context_s *context)
#define VFS_DESTROY_CONTEXT(n)  error_t (n) (struct vfs_context_s *context)
#define VFS_READ_ROOT(n)        error_t (n) (struct vfs_context_s *context, struct vfs_node_s *root)
#define VFS_WRITE_ROOT(n)       error_t (n) (struct vfs_context_s *context, struct vfs_node_s *root)

typedef VFS_CREATE_CONTEXT(vfs_create_context_t);
typedef VFS_DESTROY_CONTEXT(vfs_destroy_context_t);
typedef VFS_READ_ROOT(vfs_read_root_t);
typedef VFS_WRITE_ROOT(vfs_write_root_t);

struct vfs_context_op_s
{
	vfs_create_context_t *create;
	vfs_destroy_context_t *destroy;
	vfs_read_root_t *read_root;
	vfs_write_root_t *write_root;
};

#define VFS_INIT_NODE(n)     error_t (n) (struct vfs_node_s *node)
#define VFS_CREATE_NODE(n)   error_t (n) (struct vfs_node_s *parent, struct vfs_node_s *node)
#define VFS_LOOKUP_NODE(n)   error_t (n) (struct vfs_node_s *parent, struct vfs_node_s *node)
#define VFS_WRITE_NODE(n)    error_t (n) (struct vfs_node_s *node)
#define VFS_RELEASE_NODE(n)  error_t (n) (struct vfs_node_s *node)
#define VFS_UNLINK_NODE(n)   error_t (n) (struct vfs_node_s *node)
#define VFS_STAT_NODE(n)     error_t (n) (struct vfs_node_s *node)
#define VFS_TRUNC_NODE(n)    error_t (n) (struct vfs_node_s *node)

typedef VFS_INIT_NODE(vfs_init_node_t);
typedef VFS_CREATE_NODE(vfs_create_node_t);
typedef VFS_LOOKUP_NODE(vfs_lookup_node_t);
typedef VFS_WRITE_NODE(vfs_write_node_t);
typedef VFS_RELEASE_NODE(vfs_release_node_t);
typedef VFS_UNLINK_NODE(vfs_unlink_node_t);
typedef VFS_STAT_NODE(vfs_stat_node_t);
typedef VFS_TRUNC_NODE(vfs_trunc_node_t);

struct vfs_node_op_s
{
	vfs_init_node_t *init;
	vfs_create_node_t *create;
	vfs_lookup_node_t *lookup;
	vfs_write_node_t *write;
	vfs_release_node_t *release;
	vfs_unlink_node_t *unlink;
	vfs_stat_node_t *stat;
	vfs_trunc_node_t *trunc;
};

#define VFS_OPEN_FILE(n)    error_t (n) (struct vfs_node_s *node, struct vfs_file_s *file)
#define VFS_READ_FILE(n)    ssize_t (n) (struct vfs_file_s *file, struct ku_obj *buffer)
#define VFS_WRITE_FILE(n)   ssize_t (n) (struct vfs_file_s *file, struct ku_obj *buffer)
#define VFS_LSEEK_FILE(n)   error_t (n) (struct vfs_file_s *file)
#define VFS_CLOSE_FILE(n)   error_t (n) (struct vfs_file_s *file)
#define VFS_RELEASE_FILE(n) error_t (n) (struct vfs_file_s *file)
#define VFS_READ_DIR(n)     error_t (n) (struct vfs_file_s *file, struct ku_obj *dirent)
#define VFS_MMAP_FILE(n)    error_t (n) (struct vfs_file_s *file, struct vm_region_s *region)
#define VFS_MUNMAP_FILE(n)  error_t (n) (struct vfs_file_s *file, struct vm_region_s *region)

typedef VFS_OPEN_FILE(vfs_open_file_t);
typedef VFS_READ_FILE(vfs_read_file_t);
typedef VFS_WRITE_FILE(vfs_write_file_t);
typedef VFS_LSEEK_FILE(vfs_lseek_file_t);
typedef VFS_CLOSE_FILE(vfs_close_file_t);
typedef VFS_RELEASE_FILE(vfs_release_file_t);
typedef VFS_READ_DIR(vfs_read_dir_t);
typedef VFS_MMAP_FILE(vfs_mmap_file_t);
typedef VFS_MUNMAP_FILE(vfs_munmap_file_t);

struct vfs_file_op_s
{
	vfs_open_file_t *open;
	vfs_read_file_t *read;
	vfs_write_file_t *write;
	vfs_lseek_file_t *lseek;
	vfs_read_dir_t *readdir;
	vfs_close_file_t *close;
	vfs_release_file_t *release;
	vfs_mmap_file_t *mmap;
	vfs_munmap_file_t *munmap;
};

/* Default/generic methods */
VFS_MMAP_FILE(vfs_default_mmap_file);
VFS_MMAP_FILE(vfs_default_munmap_file);
VFS_READ_FILE(vfs_default_read);
VFS_WRITE_FILE(vfs_default_write);

/** Kernel VFS Daemon */
extern void* kvfsd(void*);

/** Initialize VFS Sub-System */
error_t vfs_init(struct device_s *device,
		 uint_t fs_type,
		 uint_t node_nr,
		 uint_t file_nr,
		 struct vfs_node_s **root);

/** Generic file related operations */
error_t vfs_create(struct vfs_node_s *cwd,
		   struct ku_obj *path,
		   uint_t flags,
		   uint_t mode,
		   struct vfs_file_s **file);

error_t vfs_open(struct vfs_node_s *cwd,
		 struct ku_obj *path,
		 uint_t flags,
		 uint_t mode,
		 struct vfs_file_s **file);

ssize_t vfs_read(struct vfs_file_s *file, struct ku_obj *buff);
ssize_t vfs_write (struct vfs_file_s *file, struct ku_obj *buff);
error_t vfs_lseek(struct vfs_file_s *file, size_t offset, uint_t whence, size_t *new_offset_ptr);
error_t vfs_close(struct vfs_file_s *file, uint_t *refcount);
error_t vfs_unlink(struct vfs_node_s *cwd, struct ku_obj *path);
error_t vfs_stat(struct vfs_node_s *cwd, struct ku_obj *path, struct vfs_node_s **node);

/** Generic directory related operations */
error_t vfs_opendir(struct vfs_node_s *cwd, struct ku_obj *path, uint_t mode, struct vfs_file_s **file);
error_t vfs_readdir(struct vfs_file_s *file, struct ku_obj *dirent);
error_t vfs_mkdir(struct vfs_node_s *cwd, struct ku_obj *path, uint_t mode);
error_t vfs_chdir(struct ku_obj *path, struct vfs_node_s *cwd, struct vfs_node_s **new_cwd);
error_t vfs_closedir(struct vfs_file_s *file, uint_t *refcount);

/** Generic FIFO operations */
error_t vfs_pipe(struct vfs_file_s *pipefd[2]);
error_t vfs_mkfifo(struct vfs_node_s *cwd, struct ku_obj *path, uint_t mode);

#endif	/* _VFS_H_ */
