/*
 * kern/task.h - task related management
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

#ifndef _TASK_H_
#define _TASK_H_

#include <types.h>
#include <list.h>
#include <bits.h>
#include <spinlock.h>
#include <atomic.h>
#include <rwlock.h>
#include <vmm.h>
#include <signal.h>
#include <mcs_sync.h>

struct vfs_node_s;
struct boot_info_s;
struct fd_info_s;
struct cluster_s;
struct dqdt_attr_s;
struct cpu_s;

#define TASK_CREATE     1
#define TASK_READY      2
#define TASK_ZOMBIE     3

struct task_s
{
	/* Various Locks */
	mcs_lock_t block;
	spinlock_t lock;
	spinlock_t th_lock;
	struct rwlock_s cwd_lock;
	spinlock_t tm_lock; 

	/* Memory Management */
	struct vmm_s vmm;

	/* Placement Info */
	struct cluster_s *cluster;
	struct cpu_s *cpu;

	/* File system */
	struct vfs_node_s *vfs_root;
	struct vfs_node_s *vfs_cwd;
	struct fd_info_s  *fd_info;
	struct vfs_file_s *bin;
  
	/* Task management */
	pid_t pid;
	uid_t uid;
	gid_t gid;
	uint_t state;
	atomic_t childs_nr;
	uint16_t childs_limit;
	struct task_s *parent;
	struct list_entry children; 
	struct list_entry list;

	/* Threads */
	uint_t threads_count;
	uint_t threads_nr;
	uint_t threads_limit;
	uint_t next_order;
	uint_t max_order;
	BITMAP_DECLARE(bitmap, (CONFIG_PTHREAD_THREADS_MAX >> 3));
	struct thread_s **th_tbl;
	struct list_entry th_root;
	struct page_s *th_tbl_pg;

	/* Signal management */
	struct sig_mgr_s sig_mgr;

#if CONFIG_FORK_LOCAL_ALLOC
	struct cluster_s *current_clstr;
#endif
};

/* Task-Management Operations */
error_t task_bootstrap_init(struct boot_info_s *info);
error_t task_bootstrap_finalize(struct boot_info_s *info);
error_t task_bootstrap_replicate(struct boot_info_s *info);
void task_manager_init(void);
struct task_s* task_lookup(uint_t pid);
error_t task_pid_alloc(uint_t *new_pid);
error_t task_create(struct task_s **new_task, struct dqdt_attr_s *attr, uint_t mode);
error_t task_dup(struct task_s *dst, struct task_s *src);
void task_default_placement(struct dqdt_attr_s *attr);
error_t task_load_init(struct task_s *task);
void task_destroy(struct task_s *task);
void task_signal_handler(struct task_s *task);

typedef error_t exec_copy_t(void *dst, void *src, uint_t count);
typedef error_t exec_strlen_t(char *dst, uint_t *count);

error_t do_exec(struct task_s *task, 
		char *path_name, 
		char **argv, 
		char **envp,
		uint_t *isFatal,
		struct thread_s **new,
		exec_copy_t ecopy, 
		exec_strlen_t estrlen);

int sys_getpid();
int sys_fork(uint_t flags, uint_t cpu_gid);
int sys_exec(char *filename, char **argv, char **envp);

/* File-Management Operations */
void task_fd_put(struct task_s *task, uint_t fd);
struct vfs_file_s* task_fd_lookup(struct task_s *task, uint_t fd);
void task_fd_set(struct task_s *task, uint_t fd, struct vfs_file_s *file);
error_t task_fd_get(struct task_s *task, uint_t *new_fd, uint_t limit);

/* Memory-Management Operations */
inline void* task_vaddr2paddr(struct task_s *task, void *vma);


/* KMEM Objects Init */
KMEM_OBJATTR_INIT(task_fdinfo_kmem_init);
KMEM_OBJATTR_INIT(task_kmem_init);

////////////////////////////////////
//        Private Section         //
////////////////////////////////////

/* for debug only */
#define TASK_PS_TRACE_OFF    0
#define TASK_PS_TRACE_ON     1
#define TASK_PS_SHOW         2

int sys_ps(uint_t cmd, uint_t pid, uint_t tid);

////////////////////////////////////

#endif	/* _TASK_H_ */
