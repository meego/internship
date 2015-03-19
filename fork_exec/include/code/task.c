/*
 * kern/task.c - task related management
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

#include <types.h>
#include <errno.h>
#include <bits.h>
#include <kmem.h>
#include <page.h>
#include <vmm.h>
#include <kdmsg.h>
#include <vfs.h>
#include <cpu.h>
#include <thread.h>
#include <list.h>
#include <scheduler.h>
#include <spinlock.h>
#include <dqdt.h>
#include <cluster.h>
#include <pmm.h>
#include <boot-info.h>
#include <task.h>

#define TASK_PID_BUSY        0x2
#define TASK_PID_BUSY_BITS   0x2

struct vfs_file_s;

struct fd_info_s
{
	spinlock_t lock;
	uint_t first;
	uint_t last;
	uint_t count;
	uint_t fds_per_tbl;
	struct vfs_file_s **fd_tbls[16];
	struct page_s *pgs_tbl[16];
};

error_t task_fd_init(struct task_s *task)
{
	kmem_req_t req;
	struct fd_info_s *info;
	uint_t count,i;
	uint_t fds_per_tbl;

	req.type  = KMEM_FDINFO;
	req.flags = AF_KERNEL | AF_ZERO;
	req.size  = sizeof(*info);
	info      = kmem_alloc(&req);

	if(info == NULL)
		return ENOMEM;

	count = (CONFIG_TASK_FILE_MAX_NR * sizeof(struct vfs_file_s*)) >> PMM_PAGE_SHIFT;

	if(count > 16)
		return ERANGE;

	fds_per_tbl = PMM_PAGE_SIZE / sizeof(struct vfs_file_s*);
	req.type    = KMEM_PAGE;
	req.size    = 0;

	for(i = 0; i < count; i++)
	{
		info->pgs_tbl[i] = kmem_alloc(&req);

		if(info->pgs_tbl[i] == NULL)
			goto fail_nomem;

		info->fd_tbls[i] = ppm_page2addr(info->pgs_tbl[i]);
	}

	spinlock_init(&info->lock, "Task FDs");
	info->first = 0;
	info->last  = 2;	/* stdin, stdout, stderr */
	info->count = count;
	info->fds_per_tbl = fds_per_tbl;
	task->fd_info = info;

	return 0;

fail_nomem:
	count = i;

	for(i = 0; i < count; i++)
	{
		req.ptr = info->pgs_tbl[i];
		kmem_free(&req);
	}

	req.type = KMEM_FDINFO;
	req.ptr  = info;
	kmem_free(&req);

	return ENOMEM;
}

void task_fd_destroy(struct task_s *task)
{
	struct fd_info_s *info;
	struct vfs_file_s *file;
	kmem_req_t req;
	uint_t i;

	req.type = KMEM_PAGE;
	info     = task->fd_info;

	for(i=0; i <= info->last; i++)
	{
		file = task_fd_lookup(task, i);

		if(file != NULL)
			vfs_close(file, NULL);
	}

	for(i = 0; i < info->count; i++)
	{
		req.ptr = info->pgs_tbl[i];
		kmem_free(&req);
	}

	req.type = KMEM_FDINFO;
	req.ptr  = info;
	kmem_free(&req);

	task->fd_info = NULL;
}

error_t task_fd_get(struct task_s *task, uint_t *new_fd, uint_t limit)
{
	register struct fd_info_s *info;
	register struct vfs_file_s **tbl;
	register uint_t fds_per_tbl;
	register uint_t fd, count, i;
	register bool_t isFound;
	error_t err;

	info  = task->fd_info;
	count = info->count;
	fds_per_tbl = info->fds_per_tbl;
	isFound = false;

	spinlock_lock(&info->lock);

	fd = info->first;

	while((fd < limit) && ((fd / fds_per_tbl) < count) && (isFound == false))
	{
		tbl = info->fd_tbls[fd / fds_per_tbl];
		i   = fd % info->fds_per_tbl;

		for(; ((tbl[i] != NULL) && (fd < limit) && (i < fds_per_tbl)); i++, fd++)
			;

		if(tbl[i] == NULL)
		{
			isFound  = true;
			tbl[i] = (struct vfs_file_s *) 0x1; /* reserved */
		}
	}

	if(isFound == true)
	{
		info->first = fd;
		info->last  = (fd > info->last) ? fd : info->last;
		err = 0;
	}
	else
		err = -1;

	spinlock_unlock(&info->lock);

	*new_fd = fd;
	return err;
}

void task_fd_put(struct task_s *task, uint_t fd)
{
	struct fd_info_s *info;
	struct vfs_file_s **tbl;
	uint_t i;

	info = task->fd_info;
	tbl  = info->fd_tbls[fd / info->fds_per_tbl];
	i    = fd % info->fds_per_tbl;

	spinlock_lock(&info->lock);

	if(fd < info->first)
		info->first = fd;

	tbl[i] = NULL;

	spinlock_unlock(&info->lock);
}


struct vfs_file_s* task_fd_lookup(struct task_s *task, uint_t fd)
{
	struct fd_info_s *info;
	struct vfs_file_s **tbl;

	info = task->fd_info;
	tbl  = info->fd_tbls[fd / info->fds_per_tbl];

	return tbl[fd % info->fds_per_tbl];
}

void task_fd_set(struct task_s *task, uint_t fd, struct vfs_file_s *file)
{
	struct fd_info_s *info;
	struct vfs_file_s **tbl;

	info = task->fd_info;
	tbl  = info->fd_tbls[fd / info->fds_per_tbl];
	tbl[fd % info->fds_per_tbl] = file;
}

static void task_fd_fork(struct task_s *dst, struct task_s *src)
{
	struct fd_info_s *dst_info;
	register struct fd_info_s *src_info;
	register struct vfs_file_s *file;
	register uint_t i;

	src_info = src->fd_info;
	dst_info = dst->fd_info;

	spinlock_lock(&src_info->lock);

	for(i=0; i <= src_info->last; i++)
	{
		file = task_fd_lookup(src, i);

		if(file != NULL)
		{
			atomic_add(&file->f_count, 1);
			task_fd_set(dst, i, file);
		}
	}

	dst_info->first = src_info->first;
	dst_info->last  = src_info->last;

	spinlock_unlock(&src_info->lock);
}


static struct task_s task0;

/* TODO: use atomic counter instead of spinlock */
struct tasks_manager_s
{
	atomic_t tm_next_clstr;
	atomic_t tm_next_cpu;
	spinlock_t tm_lock;
	uint_t tm_last_pid;
	struct task_s *tm_tbl[CONFIG_TASK_MAX_NR];
};

static struct tasks_manager_s tasks_mgr = 
{
	.tm_next_clstr = ATOMIC_INITIALIZER,
	.tm_next_cpu = ATOMIC_INITIALIZER,
	.tm_tbl[0] = &task0
};

void task_manager_init(void)
{
	spinlock_init(&tasks_mgr.tm_lock, "Tasks Mgr");
	tasks_mgr.tm_last_pid = 1;
	memset(&tasks_mgr.tm_tbl[1], 
	       0, 
	       sizeof(struct task_s*) * (CONFIG_TASK_MAX_NR - 1));
}

void task_default_placement(struct dqdt_attr_s *attr)
{
	uint_t cid;
	uint_t cpu_lid;
  
	cid           = atomic_inc(&tasks_mgr.tm_next_clstr);
	cpu_lid       = atomic_inc(&tasks_mgr.tm_next_cpu);
	cid          %= arch_onln_cluster_nr();  
	cpu_lid      %= current_cluster->cpu_nr;
	attr->cid     = cid;
	attr->cpu_id  = cpu_lid;
}

struct task_s* task_lookup(uint_t pid)
{
	struct task_s *task;

	if(pid >= CONFIG_TASK_MAX_NR)
		return NULL;

	task = tasks_mgr.tm_tbl[pid];

	return ((uint_t)task == TASK_PID_BUSY) ? NULL : task;
}

error_t task_pid_alloc(uint_t *new_pid)
{
	register uint_t pid;
	register uint_t overlap;
	error_t err;

	err     = EAGAIN;
	overlap = false;

	spinlock_lock(&tasks_mgr.tm_lock);
	pid = tasks_mgr.tm_last_pid;
  
	while(tasks_mgr.tm_tbl[pid] != NULL)
	{    
		if((++pid) == CONFIG_TASK_MAX_NR)
		{
			pid = 0;
			if(overlap == true)
				break;
			overlap = true;
		}
	}
  
	if(tasks_mgr.tm_tbl[pid] == NULL)
	{
		tasks_mgr.tm_tbl[pid] = (void*) TASK_PID_BUSY;
		err                   = 0;
		*new_pid              = pid;
	}

	tasks_mgr.tm_last_pid = pid;
	spinlock_unlock(&tasks_mgr.tm_lock);
	return err;
}

inline void* task_vaddr2paddr(struct task_s* task, void *vma)
{
	uint_t paddr;
	error_t err;
	pmm_page_info_t info;

	err = pmm_get_page(&task->vmm.pmm, (vma_t)vma, &info);
  
	if((err) || ((info.attr & PMM_PRESENT) == 0))
	{
		printk(WARNING, "WARNING: %s: core %d, pid %d, tid %d, vma 0x%x, err %d, attr 0x%x, ppn 0x%x\n",
		       __FUNCTION__,
		       cpu_get_id(),
		       task->pid,
		       current_thread->info.order,
		       vma,
		       err,
		       info.attr,
		       info.ppn);

		return NULL;
	}

	paddr = (info.ppn << PMM_PAGE_SHIFT) | ((vma_t)vma & PMM_PAGE_MASK);

	return (void*) paddr;
}

static void task_ctor(struct kcm_s *kcm, void *ptr);

error_t task_bootstrap_init(struct boot_info_s *info)
{
	task_ctor(NULL, &task0);
	memset(&task0.vmm, 0, sizeof(task0.vmm));
	task0.vmm.text_start  = CONFIG_KERNEL_OFFSET;
	task0.vmm.limit_addr  = CONFIG_KERNEL_OFFSET;
	//FIXME: the following should removed
	task0.vmm.devreg_addr = 0XFFFFFFFF;//CONFIG_DEVREGION_OFFSET; 
	pmm_bootstrap_init(&task0.vmm.pmm, info->boot_pgdir);
	/* Nota: vmm is not intialized */
	task0.vfs_root        = NULL;
	task0.vfs_cwd         = NULL;
	task0.fd_info         = NULL;
	task0.bin             = NULL;
	task0.threads_count   = 0;
	task0.threads_nr      = 0;
	task0.threads_limit   = CONFIG_PTHREAD_THREADS_MAX;
	task0.pid             = 0;
	atomic_init(&task0.childs_nr, 0);
	task0.childs_limit    = CONFIG_TASK_CHILDS_MAX_NR;
	return 0;
}

error_t task_bootstrap_finalize(struct boot_info_s *info)
{
	register error_t err;
  
	err = vmm_init(&task0.vmm);
  
	if(err) return err;

	err =  pmm_init(&task0.vmm.pmm, current_cluster);

	if(err)
	{
		printk(ERROR,"ERROR: %s: Failed, err %d\n", __FUNCTION__, err);
		while(1);
	}

	return 0;
}

error_t task_create(struct task_s **new_task, struct dqdt_attr_s *attr, uint_t mode)
{
	struct task_s *task;
	kmem_req_t req;
	uint_t pid = 0;
	uint_t err;
  
	assert(mode != CPU_SYS_MODE);

	if((err=task_pid_alloc(&pid)))
	{
		printk(WARNING, "WARNING: %s: System Is Out of PIDs\n", __FUNCTION__);
		return err;
	}

	req.type  = KMEM_TASK;
	req.size  = sizeof(struct task_s);
	req.flags = AF_KERNEL | AF_REMOTE;
	//FIXME: add remote create support ?
	req.ptr   = current_cluster; //req.ptr   = attr->cluster;
  
	if((task = kmem_alloc(&req)) == NULL)
	{
		err = ENOMEM;
		goto fail_task_desc;
	}

	if((err = signal_manager_init(task)) != 0)
		goto fail_signal_mgr;

	req.type = KMEM_PAGE;
	req.size = (CONFIG_PTHREAD_THREADS_MAX * sizeof(struct thread_s*)) / PMM_PAGE_SIZE;
	req.size = (req.size == 0) ? 0 : bits_log2(req.size);
  
	task->th_tbl_pg = kmem_alloc(&req);

	if(task->th_tbl_pg == NULL)
	{
		err = ENOMEM;
		goto fail_th_tbl;
	}

	task->th_tbl = ppm_page2addr(task->th_tbl_pg);
  
	err = task_fd_init(task);

	if(err)
		goto fail_fd_info;

	memset(&task->vmm, 0, sizeof(task->vmm));

	//FIXME: add remote create support: modify the task struct
	task->cluster         = current_cluster;//attr->cluster;
	task->cpu             = cpu_lid2ptr(attr->cpu_id);//attr->cpu;
	task->bin             = NULL;
	task->threads_count   = 0;
	task->threads_nr      = 0;
	task->threads_limit   = CONFIG_PTHREAD_THREADS_MAX;
	bitmap_set_range(task->bitmap, 0, CONFIG_PTHREAD_THREADS_MAX);
	task->pid             = pid;
	task->state           = TASK_CREATE;
	atomic_init(&task->childs_nr, 0);
	task->childs_limit    = CONFIG_TASK_CHILDS_MAX_NR;
	*new_task             = task;
	tasks_mgr.tm_tbl[pid] = task;
	cpu_wbflush();
	return 0;

fail_fd_info:
	req.type = KMEM_PAGE;
	req.ptr  = task->th_tbl_pg;
	kmem_free(&req);

fail_signal_mgr:
fail_th_tbl:
	req.type = KMEM_TASK;
	req.ptr  = task;
	kmem_free(&req);

fail_task_desc:
	tasks_mgr.tm_tbl[pid] = NULL;
	*new_task = NULL;
	return err;
}

error_t task_dup(struct task_s *dst, struct task_s *src)
{
	rwlock_wrlock(&src->cwd_lock);

	vfs_node_up_atomic(src->vfs_root);
	vfs_node_up_atomic(src->vfs_cwd);

	dst->vfs_root = src->vfs_root;
	dst->vfs_cwd  = src->vfs_cwd;

	rwlock_unlock(&src->cwd_lock);
  
	task_fd_fork(dst, src);

	assert(src->bin != NULL);
	(void)atomic_add(&src->bin->f_count, 1);
	dst->bin = src->bin;
	return 0;
}

void task_destroy(struct task_s *task)
{
	kmem_req_t req;
	register uint_t pid;
	uint_t pgfault_nr;
	uint_t spurious_pgfault_nr;
	uint_t remote_pages_nr;
	uint_t u_err_nr;
	uint_t m_err_nr;

	assert(task->threads_nr == 0 && 
	       "Unexpected task destruction, One or more Threads still active\n");

	pid = task->pid;

	signal_manager_destroy(task);

	tasks_mgr.tm_tbl[task->pid] = NULL;
	cpu_wbflush();

	task_fd_destroy(task);

	if(task->bin != NULL)
		vfs_close(task->bin, NULL);

	vfs_node_down_atomic(task->vfs_root);
	vfs_node_down_atomic(task->vfs_cwd);

	pgfault_nr          = task->vmm.pgfault_nr;
	spurious_pgfault_nr = task->vmm.spurious_pgfault_nr;
	remote_pages_nr     = task->vmm.remote_pages_nr;
	u_err_nr            = task->vmm.u_err_nr;
	m_err_nr            = task->vmm.m_err_nr;

	vmm_destroy(&task->vmm);
	pmm_release(&task->vmm.pmm);
	pmm_destroy(&task->vmm.pmm);

	req.type = KMEM_PAGE;
	req.ptr  = task->th_tbl_pg;
	kmem_free(&req);

	req.type = KMEM_FDINFO;
	req.ptr  = task->fd_info;
	kmem_free(&req);
  
	req.type = KMEM_TASK;
	req.ptr  = task;
	kmem_free(&req);

	printk(INFO, "INFO: %s: pid %d [ %d, %d, %d, %d, %d ]\n",
	       __FUNCTION__, 
	       pid, 
	       pgfault_nr,
	       spurious_pgfault_nr,
	       remote_pages_nr,
	       u_err_nr,
	       m_err_nr);
}


static void task_ctor(struct kcm_s *kcm, void *ptr)
{
	struct task_s *task = ptr;
  
	mcs_lock_init(&task->block, "Task");
	spinlock_init(&task->lock, "Task");
	rwlock_init(&task->cwd_lock);
	spinlock_init(&task->th_lock, "Task threads");
	spinlock_init(&task->tm_lock, "Task Time");
	list_root_init(&task->children);
	list_root_init(&task->th_root);
}

KMEM_OBJATTR_INIT(task_kmem_init)
{
	attr->type   = KMEM_TASK;
	attr->name   = "KCM Task";
	attr->size   = sizeof(struct task_s);
	attr->aligne = 0;
	attr->min    = CONFIG_TASK_KCM_MIN;
	attr->max    = CONFIG_TASK_KCM_MAX;
	attr->ctor   = task_ctor;
	attr->dtor   = NULL;
	return 0;
}

KMEM_OBJATTR_INIT(task_fdinfo_kmem_init)
{
	attr->type   = KMEM_FDINFO;
	attr->name   = "KCM FDINFO";
	attr->size   = sizeof(struct fd_info_s);
	attr->aligne = 0;
	attr->min    = CONFIG_FDINFO_KCM_MIN;
	attr->max    = CONFIG_FDINFO_KCM_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}
