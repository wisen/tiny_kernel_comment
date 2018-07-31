#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <linux/auxvec.h>
#include <linux/types.h>
#include <linux/threads.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/page-debug-flags.h>
#include <linux/uprobes.h>
#include <linux/page-flags-layout.h>
#include <asm/page.h>
#include <asm/mmu.h>

#ifndef AT_VECTOR_SIZE_ARCH
#define AT_VECTOR_SIZE_ARCH 0
#endif
#define AT_VECTOR_SIZE (2*(AT_VECTOR_SIZE_ARCH + AT_VECTOR_SIZE_BASE + 1))

struct address_space;

#define USE_SPLIT_PTE_PTLOCKS	(NR_CPUS >= 4/*CONFIG_SPLIT_PTLOCK_CPUS=4*/)
#define USE_SPLIT_PMD_PTLOCKS	(USE_SPLIT_PTE_PTLOCKS && \
		IS_ENABLED(CONFIG_ARCH_ENABLE_SPLIT_PMD_PTLOCK))
#define ALLOC_SPLIT_PTLOCKS	(SPINLOCK_SIZE > BITS_PER_LONG/8)

/*
 * Each physical page in the system has a struct page associated with
 * it to keep track of whatever it is we are using the page for at the
 * moment. Note that we have no way to track which tasks are using
 * a page, though if it is a pagecache page, rmap structures can tell us
 * who is mapping it.
 * wisen: ÿ������ҳ����һ����Ӧ��page struct������:��¼����״̬������ͻ��գ�
 * wisen: �����Լ�ͬ������.����û�취�����ĸ�task����ʹ��ҳ�����������һ��
 * wisen: pagecache��rmap�ṹ����Ը�������˭��ӳ����.
 *
 * The objects in struct page are organized in double word blocks in
 * order to allows us to use atomic double word operations on portions
 * of struct page. That is currently only used by slub but the arrangement
 * allows the use of atomic double word operations on the flags/mapping
 * and lru list pointers also.
 */
struct page {
	/* First double word block */
    // wisen comment
   // Page flags: | [SECTION] | [NODE] | ZONE | [LAST_CPUPID] | ... | FLAGS |
   // flag�и�λ����
   // -->��λ����                ��λ����<--
   // |section|node  |zone  |.....| flags  |
   // ��Ҫ��Ϊ4���֣����б�־λflag���λ����������λ�ֶ����λ�������м���ڿ���λ��
   //  section����Ҫ����ϡ���ڴ�ģ��SPARSEMEM���ɺ��ԡ�
   //  node��NUMA�ڵ�ţ���ʶ��page������һ���ڵ㡣
   //  zone���ڴ����־����ʶ��page������һ��zone��
   //  flag��page��״̬��ʶ�����õ��У�
   //     PG_locked��page��������˵����ʹ�������ڲ�����page��
   //     PG_error��״̬��־����ʾ�漰��page��IO���������˴���
   //     PG_referenced����ʾpage�ոձ����ʹ���
   //     PG_active��page����inactive LRU����PG_active��PG_referencedһ����Ƹ�page�Ļ�Ծ�̶ȣ������ڴ����ʱ����ǳ����á�
   //     PG_uptodate����ʾpage�������Ѿ���󱸴洢����ͬ���ģ������µġ�
   //     PG_dirty����󱸴洢���е�������ȣ���page�������Ѿ����޸ġ�
   //     PG_lru����ʾ��page����LRU�����ϡ�
   //     PG_slab����page����slab��������
   //     PG_reserved�����øñ�־����ֹ��page��������swap��
   //     PG_private�����page�е�private��Ա�ǿգ�����Ҫ���øñ�־���ο�6)��private�Ľ��͡�
   //     PG_writeback��page�е��������ڱ���д���󱸴洢����
   //     PG_swapcache����ʾ��page����swap cache�С�
   //     PG_mappedtodisk����ʾpage�е������ں󱸴洢�����ж�Ӧ��
   //     PG_reclaim����ʾ��pageҪ�����ա���PFRA����Ҫ����ĳ��page����Ҫ���øñ�־��
   //     PG_swapbacked����page�ĺ󱸴洢����swap��
   //     PG_unevictable����page����ס�����ܽ��������������LRU_UNEVICTABLE�����У��������ļ���page��ramdisk��ramfsʹ�õ�ҳ��
   //         shm_locked��mlock������ҳ��
   //     PG_mlocked����page��vma�б�������һ����ͨ��ϵͳ����mlock()������һ���ڴ档

   // �ں����ṩ��һЩ��׼�꣬������顢����ĳЩ�ض��ı���λ���磺
   //     -> PageXXX(page)�����page�Ƿ�������PG_XXXλ
   //     -> SetPageXXX(page)������page��PG_XXXλ
   //     -> ClearPageXXX(page)�����page��PG_XXXλ
   //     -> TestSetPageXXX(page)������page��PG_XXXλ��������ԭֵ
   //     -> TestClearPageXXX(page)�����page��PG_XXXλ��������ԭֵ
	//PG_active,PG_reference���ڱ�ʾ��ǰҳ�Ļ�Ծ״̬���������Ƿ����
	//PG_unevictable��ʾ��ǰҳ�����Ի���
	//PG_mlocked��ʾ��ǰҳ��ϵͳ����mlock()�����ˣ���ֹ�������ͷ�
	//PG_lru��ʾ��ǰҳ����lru������
	//PG_swapcache��ʾ��ǰҳ���ڱ�����/����
	//PG_private��PG_private_2�ֱ�������ʾһ��zspage�ĵ�һ��ҳ�����һ��ҳ
	unsigned long flags;		/* Atomic flags, some possibly
					 * updated asynchronously */
	//ҳ����ı��ʾ���һ������ҳ�����ÿ��ҳ��������ͨ��mmaping��index�����ֶ������
	//������й�����mmapingָ��ҳ�����������е�address_space����index��ʾ��ҳ��СΪ
	//��λ��ƫ��������ƫ������ʾҳ���������ڴ����ļ��е�ƫ������
	union {
    //mapping�����ֺ���
    //    a: ���mapping = 0��˵����page���ڽ������棨swap cache����
    //        ����Ҫʹ�õ�ַ�ռ�ʱ��ָ�����������ĵ�ַ�ռ�swapper_space��
    //    b: ���mapping != 0��bit[0] = 0��˵����page����ҳ������ļ�ӳ�䣬
    //        mappingָ���ļ��ĵ�ַ�ռ�address_space��
    //    c: ���mapping != 0��bit[0] != 0��˵����pageΪ����ӳ�䣬mappingָ��
    //        struct anon_vma����
    //    ͨ��mapping�ָ�anon_vma�ķ�����anon_vma = (struct anon_vma *)(mapping - PAGE_MAPPING_ANON)��
    // ����mappingָ��ĵ�2λ���ã����Բο�PageAnon��ʵ�֣�������2λû�б���λ����ô������file page������λ�˾�Ӧ����anon page
		struct address_space *mapping;	/* If low bit clear, points to
						 * inode address_space, or NULL.
						 * If page mapped as anonymous
						 * memory, low bit is set, and
						 * it points to anon_vma object:
						 * see PAGE_MAPPING_ANON below.
						 */
		void *s_mem;			/* slab first object */
	};
	//wisen: mappingָ����ҳ�����ڵĵ�ַ�ռ�

	//wisen: index��ҳ����mappingӳ���ڲ���ƫ����
	/* Second double word */
	struct {
		union {
    //index��ӳ�������ռ䣨vma_area���ڵ�ƫ�ƣ�һ���ļ�����ֻӳ��һ���֣�����ӳ����1M��
    //�ռ䣬indexָ������1M�ռ��ڵ�ƫ�ƣ��������������ļ��ڵ�ƫ�ơ�

			pgoff_t index;		/* Our offset within mapping. */
			void *freelist;		/* sl[aou]b first free object */
			bool pfmemalloc;	/* If set by the page allocator,
						 * ALLOC_NO_WATERMARKS was set
						 * and the low watermark was not
						 * met implying that the system
						 * is under some pressure. The
						 * caller should try ensure
						 * this page is only used to
						 * free other pages.
						 */
		};

		union {
#if defined(CONFIG_HAVE_CMPXCHG_DOUBLE) && \
	defined(CONFIG_HAVE_ALIGNED_STRUCT_PAGE)
			/* Used for cmpxchg_double in slub */
			unsigned long counters;
#else
			/*
			 * Keep _count separate from slub cmpxchg_double data.
			 * As the rest of the double word is protected by
			 * slab_lock but _count is not.
			 */
			unsigned counters;
#endif

			struct {

				union {
					/*
					 * Count of ptes mapped in
					 * mms, to show when page is
					 * mapped & limit reverse map
					 * searches.
					 *
					 * Used also for tail pages
					 * refcounting instead of
					 * _count. Tail pages cannot
					 * be mapped and keeping the
					 * tail page _count zero at
					 * all times guarantees
					 * get_page_unless_zero() will
					 * never succeed on tail
					 * pages.
					 */
					atomic_t _mapcount;
					//wisen: ԭ�Ӽ�����Ա_mapcount��ʾ��ҳ�����ж���ҳָ���ҳ��
                   //wisen: ������ҳ��ӳ��Ĵ�����Ҳ����˵��pageͬʱ�����ٸ����̹���
                    //wisen: ��ʼֵΪ-1�����ֻ��һ�����̵�ҳ��ӳ���ˣ���ֵΪ0 ��
                    //�����page���ڻ��ϵͳ�У���ֵΪPAGE_BUDDY_MAPCOUNT_VALUE��-128����
                    //�ں�ͨ���жϸ�ֵ�Ƿ�ΪPAGE_BUDDY_MAPCOUNT_VALUE��ȷ����page�Ƿ�����
                    //���ϵͳ��

					struct { /* SLUB */
						unsigned inuse:16;
						unsigned objects:15;
						unsigned frozen:1;
					};
					int units;	/* SLOB */
				};
				atomic_t _count;		/* Usage count, see below. */
				//wisen: ԭ�Ӽ�����Ա_count��ָ���˵�ǰҳ������ü���������ֵΪ0ʱ
				//wisen: ��˵����û�б�ʹ�ã���ʱ���·����ڴ�ʱ���Ϳ��Ա�ʹ��
				//wisen: �ں˴���Ӧ��ͨ��page_count��������������ֱ�ӷ��ʡ�
				//wisen: ע������_count��_mapcount��_mapcount��ʾ����ӳ�������
                //wisen: ��_count��ʾ����ʹ�ô�������ӳ���˲�һ����ʹ�ã���Ҫʹ�ñ���
                //wisen: ��ӳ�䡣

			};
			unsigned int active;	/* SLAB */
		};
	};

	/* Third double word block */
	union {
    //��Ҫ��3����;��
    //    a��page���ڻ��ϵͳ��ʱ������������ͬ�׵Ļ�飨ֻʹ�û���еĵ�һ��page��lru���ɴﵽĿ�ģ���
    //    b��page����slabʱ��page->lru.nextָ��pageפ���ĵĻ���Ĺ���ṹ��
    //        page->lru.precָ�򱣴��page��slab�Ĺ���ṹ��
    //    c��page���û�̬ʹ�û򱻵���ҳ����ʹ��ʱ�����ڽ���page����zone����Ӧ��lru����
    //        ���ڴ����ʱʹ�á�
		struct list_head lru;	/* Pageout list, eg. active_list
					 * protected by zone->lru_lock !
					 * Can be used as a generic list
					 * by the page owner.
					 */
		struct {		/* slub per cpu partial pages */
			struct page *next;	/* Next partial slab */
#ifdef CONFIG_64BIT
			int pages;	/* Nr of partial slabs left */
			int pobjects;	/* Approximate # of objects */
#else
			short int pages;
			short int pobjects;
#endif
		};

		struct slab *slab_page; /* slab fields */
		struct rcu_head rcu_head;	/* Used by SLAB
						 * when destroying via RCU
						 */
#if defined(CONFIG_TRANSPARENT_HUGEPAGE) && USE_SPLIT_PMD_PTLOCKS
		pgtable_t pmd_huge_pte; /* protected by page->ptl */
#endif
	};

	/* Remainder is not double word aligned */
	union {
    
    //private˽������ָ�룬��Ӧ�ó���ȷ�������ĺ��壺
    //    a�����������PG_private��־����ʾbuffer_heads��
    //    b�����������PG_swapcache��־��private�洢�˸�page�ڽ��������ж�Ӧ��λ��
    //        ��Ϣswp_entry_t��
    //    c�����_mapcount = PAGE_BUDDY_MAPCOUNT_VALUE��˵����pageλ�ڻ��ϵͳ��
    //        private�洢�û��Ľס�
		unsigned long private;		/* Mapping-private opaque data:
					 	 * usually used for buffer_heads
						 * if PagePrivate set; used for
						 * swp_entry_t if PageSwapCache;
						 * indicates order in the buddy
						 * system if PG_buddy is set.
						 */
		//wisen: private����;��flags��־λϢϢ��ء�
		//wisen: ���������PG_private,��ô��������buffer_heads;
		//wisen: ���������PG_swapcache����ô����swp_entry_t;
		//wisen�����������PG_buddy�������ڻ��ϵͳ�еĽ�(Order)��
#if USE_SPLIT_PTE_PTLOCKS
#if ALLOC_SPLIT_PTLOCKS
		spinlock_t *ptl;
#else
		spinlock_t ptl;
#endif
#endif
		struct kmem_cache *slab_cache;	/* SL[AU]B: Pointer to slab */
		struct page *first_page;	/* Compound tail pages */
		//wisen: �ں˿��Խ�������ڵ�ҳ��ϲ�Ϊ����ҳ(Compound Page)
		//wisen: �����еĵ�һ��Ҳ��Ϊ��ҳ(Head Page)�������������ҳ����βҳ(Tail Page)
		//wisen: ����βҳ��Ӧ�Ĺ���page���ݽṹ����first_pageָ����ҳ��
	};

	/*
	 * On machines where all RAM is mapped into kernel address space,
	 * we can simply calculate the virtual address. On machines with
	 * highmem some memory is mapped into kernel virtual memory
	 * dynamically, so we need a place to store that address.
	 * Note that this field could be 16 bits on x86 ... ;)
	 *
	 * Architectures with slow multiplication can define
	 * WANT_PAGE_VIRTUAL in asm/page.h
	 */
#if 0/*defined(WANT_PAGE_VIRTUAL)*/
	void *virtual;			/* Kernel virtual address (NULL if
					   not kmapped, ie. highmem) */
#endif /* WANT_PAGE_VIRTUAL */
#ifdef CONFIG_WANT_PAGE_DEBUG_FLAGS
	unsigned long debug_flags;	/* Use atomic bitops on this */
#endif

#ifdef CONFIG_KMEMCHECK
	/*
	 * kmemcheck wants to track the status of each byte in a page; this
	 * is a pointer to such a status block. NULL if not tracked.
	 */
	void *shadow;
#endif

#ifdef LAST_CPUPID_NOT_IN_PAGE_FLAGS
	int _last_cpupid;
#endif
}
/*
 * The struct page can be forced to be double word aligned so that atomic ops
 * on double words work. The SLUB allocator can make use of such a feature.
 */
#ifdef CONFIG_HAVE_ALIGNED_STRUCT_PAGE
	__aligned(2 * sizeof(unsigned long))
#endif
;

struct page_frag {
	struct page *page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	__u32 offset;
	__u32 size;
#else
	__u16 offset;
	__u16 size;
#endif
};

typedef unsigned long __nocast vm_flags_t;

/*
 * A region containing a mapping of a non-memory backed file under NOMMU
 * conditions.  These are held in a global tree and are pinned by the VMAs that
 * map parts of them.
 */
struct vm_region {
	struct rb_node	vm_rb;		/* link in global region tree */
	vm_flags_t	vm_flags;	/* VMA vm_flags */
	unsigned long	vm_start;	/* start address of region */
	unsigned long	vm_end;		/* region initialised to here */
	unsigned long	vm_top;		/* region allocated to here */
	unsigned long	vm_pgoff;	/* the offset in vm_file corresponding to vm_start */
	struct file	*vm_file;	/* the backing file or NULL */

	int		vm_usage;	/* region usage count (access under nommu_region_sem) */
	bool		vm_icache_flushed : 1; /* true if the icache has been flushed for
						* this region */
};

/*
 * This struct defines a memory VMM memory area. There is one of these
 * per VM-area/task.  A VM area is any part of the process virtual memory
 * space that has a special rule for the page-fault handlers (ie a shared
 * library, the executable area etc).
 */
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	/*
	 * Largest free memory gap in bytes to the left of this VMA.
	 * Either between this VMA and vma->vm_prev, or between one of the
	 * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
	 * get_unmapped_area find a free area of the right size.
	 */
	unsigned long rb_subtree_gap;

	/* Second cache line starts here. */

	struct mm_struct *vm_mm;	/* The address space we belong to. */
	pgprot_t vm_page_prot;		/* Access permissions of this VMA. */
	unsigned long vm_flags;		/* Flags, see mm.h. */

	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree, or
	 * linkage of vma in the address_space->i_mmap_nonlinear list.
	 *
	 * For private anonymous mappings, a pointer to a null terminated string
	 * in the user process containing the name given to the vma, or NULL
	 * if unnamed.
	 */
	union {
		struct {
			struct rb_node rb;
			unsigned long rb_subtree_last;
		} linear;
		struct list_head nonlinear;
		const char __user *anon_name;
	} shared;

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_sem &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units, *not* PAGE_CACHE_SIZE */
	struct file * vm_file;		/* File we map to (can be NULL). */
	void * vm_private_data;		/* was vm_pte (shared mem) */

#if 0//#ifndef CONFIG_MMU//CONFIG_MMU=y
	struct vm_region *vm_region;	/* NOMMU mapping region */
#endif
#if 0
	struct mempolicy *vm_policy;	/* NUMA policy for the VMA */
#endif
};

struct core_thread {
	struct task_struct *task;
	struct core_thread *next;
};

struct core_state {
	atomic_t nr_threads;
	struct core_thread dumper;
	struct completion startup;
};

enum {
	MM_FILEPAGES,
	MM_ANONPAGES,
	MM_SWAPENTS,
	NR_MM_COUNTERS
};

#if USE_SPLIT_PTE_PTLOCKS && 1/*defined()*/
#define SPLIT_RSS_COUNTING
/* per-thread cached information, */
struct task_rss_stat {
	int events;	/* for synchronization threshold */
	int count[NR_MM_COUNTERS];
};
#endif /* USE_SPLIT_PTE_PTLOCKS */

struct mm_rss_stat {
	atomic_long_t count[NR_MM_COUNTERS];
};

struct kioctx_table;
struct mm_struct {
	struct vm_area_struct *mmap;		/* list of VMAs */
	//wisen: �������������ʱ, ���õ�������mmapָ��ָ���������
	struct rb_root mm_rb;
	//wisen: �����������ʱ, ����"�����(red_black tree)���ṹ����mm_rbָ�������
	u32 vmacache_seqnum;                   /* per-thread vmacache */
#if 1//#ifdef CONFIG_MMU//CONFIG_MMU=y
	unsigned long (*get_unmapped_area) (struct file *filp,
				unsigned long addr, unsigned long len,
				unsigned long pgoff, unsigned long flags);
#endif
	unsigned long mmap_base;		/* base of mmap area */
	unsigned long mmap_legacy_base;         /* base of mmap area in bottom-up allocations */
	unsigned long task_size;		/* size of task vm space */
	unsigned long highest_vm_end;		/* highest vma end address */
	pgd_t * pgd;
	atomic_t mm_users;			/* How many users with user space? */
	atomic_t mm_count;			/* How many references to "struct mm_struct" (users count as 1) */
	atomic_long_t nr_ptes;			/* Page table pages */
	int map_count;				/* number of VMAs */

	spinlock_t page_table_lock;		/* Protects page tables and some counters */
	struct rw_semaphore mmap_sem;

	struct list_head mmlist;		/* List of maybe swapped mm's.	These are globally strung
						 * together off init_mm.mmlist, and are protected
						 * by mmlist_lock
						 */


	unsigned long hiwater_rss;	/* High-watermark of RSS usage */
	unsigned long hiwater_vm;	/* High-water virtual memory usage */

	unsigned long total_vm;		/* Total pages mapped */
	unsigned long locked_vm;	/* Pages that have PG_mlocked set */
	unsigned long pinned_vm;	/* Refcount permanently increased */
	unsigned long shared_vm;	/* Shared pages (files) */
	unsigned long exec_vm;		/* VM_EXEC & ~VM_WRITE */
	unsigned long stack_vm;		/* VM_GROWSUP/DOWN */
	unsigned long def_flags;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long start_brk, brk, start_stack;
	unsigned long arg_start, arg_end, env_start, env_end;

	unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */

	/*
	 * Special counters, in some configurations protected by the
	 * page_table_lock, in other configurations by being atomic.
	 */
	struct mm_rss_stat rss_stat;

	struct linux_binfmt *binfmt;

	cpumask_var_t cpu_vm_mask_var;

	/* Architecture-specific MM context */
	mm_context_t context;

	unsigned long flags; /* Must use atomic bitops to access the bits */

	struct core_state *core_state; /* coredumping support */
#if 1//#ifdef //
	spinlock_t			ioctx_lock;
	struct kioctx_table __rcu	*ioctx_table;
#endif
#ifdef CONFIG_MEMCG
	/*
	 * "owner" points to a task that is regarded as the canonical
	 * user/owner of this mm. All of the following must be true in
	 * order for it to be changed:
	 *
	 * current == mm->owner
	 * current->mm != mm
	 * new_owner->mm == mm
	 * new_owner->alloc_lock is held
	 */
	struct task_struct __rcu *owner;
#endif

	/* store ref to file /proc/<pid>/exe symlink points to */
	struct file *exe_file;
#ifdef CONFIG_MMU_NOTIFIER
	struct mmu_notifier_mm *mmu_notifier_mm;
#endif
#if defined(CONFIG_TRANSPARENT_HUGEPAGE) && !USE_SPLIT_PMD_PTLOCKS
	pgtable_t pmd_huge_pte; /* protected by page_table_lock */
#endif
#ifdef CONFIG_CPUMASK_OFFSTACK
	struct cpumask cpumask_allocation;
#endif
#if 0_BALANCING
	/*
	 * numa_next_scan is the next time that the PTEs will be marked
	 * pte_numa. NUMA hinting faults will gather statistics and migrate
	 * pages to new nodes if necessary.
	 */
	unsigned long numa_next_scan;

	/* Restart point for scanning and setting pte_numa */
	unsigned long numa_scan_offset;

	/* numa_scan_seq prevents two threads setting pte_numa */
	int numa_scan_seq;
#endif
#if defined(CONFIG_NUMA_BALANCING) || 1/*defined()*/
	/*
	 * An operation with batched TLB flushing is going on. Anything that
	 * can move process memory needs to flush the TLB when moving a
	 * PROT_NONE or PROT_NUMA mapped page.
	 */
	bool tlb_flush_pending;
#endif
	struct uprobes_state uprobes_state;
};

static inline void mm_init_cpumask(struct mm_struct *mm)
{
#ifdef CONFIG_CPUMASK_OFFSTACK
	mm->cpu_vm_mask_var = &mm->cpumask_allocation;
#endif
	cpumask_clear(mm->cpu_vm_mask_var);
}

/* Future-safe accessor for struct mm_struct's cpu_vm_mask. */
static inline cpumask_t *mm_cpumask(struct mm_struct *mm)
{
	return mm->cpu_vm_mask_var;
}

#if defined(CONFIG_NUMA_BALANCING) || 1/*defined()*/
/*
 * Memory barriers to keep this state in sync are graciously provided by
 * the page table locks, outside of which no page table modifications happen.
 * The barriers below prevent the compiler from re-ordering the instructions
 * around the memory barriers that are already present in the code.
 */
static inline bool mm_tlb_flush_pending(struct mm_struct *mm)
{
	barrier();
	return mm->tlb_flush_pending;
}
static inline void set_tlb_flush_pending(struct mm_struct *mm)
{
	mm->tlb_flush_pending = true;

	/*
	 * Guarantee that the tlb_flush_pending store does not leak into the
	 * critical section updating the page tables
	 */
	smp_mb__before_spinlock();
}
/* Clearing is done after a TLB flush, which also provides a barrier. */
static inline void clear_tlb_flush_pending(struct mm_struct *mm)
{
	barrier();
	mm->tlb_flush_pending = false;
}
#else
static inline bool mm_tlb_flush_pending(struct mm_struct *mm)
{
	return false;
}
static inline void set_tlb_flush_pending(struct mm_struct *mm)
{
}
static inline void clear_tlb_flush_pending(struct mm_struct *mm)
{
}
#endif

struct vm_special_mapping
{
	const char *name;
	struct page **pages;
};

enum tlb_flush_reason {
	TLB_FLUSH_ON_TASK_SWITCH,
	TLB_REMOTE_SHOOTDOWN,
	TLB_LOCAL_SHOOTDOWN,
	TLB_LOCAL_MM_SHOOTDOWN,
	NR_TLB_FLUSH_REASONS,
};

/* Return the name for an anonymous mapping or NULL for a file-backed mapping */
static inline const char __user *vma_get_anon_name(struct vm_area_struct *vma)
{
	if (vma->vm_file)
		return NULL;

	return vma->shared.anon_name;
}

#endif /* _LINUX_MM_TYPES_H */
