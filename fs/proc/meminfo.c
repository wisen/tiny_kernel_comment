#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "internal.h"

void __attribute__((weak)) arch_report_meminfo(struct seq_file *m)
{
}

static int meminfo_proc_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	unsigned long committed;
	struct vmalloc_info vmi;
	long cached;
	long available;
	unsigned long pagecache;
	unsigned long wmark_low = 0;
	unsigned long pages[NR_LRU_LISTS];
	struct zone *zone;
	int lru;

/*
 * display in kilobytes.
 */
#define K(x) ((x) << (PAGE_SHIFT - 10))
	si_meminfo(&i);
	si_swapinfo(&i);
	committed = percpu_counter_read_positive(&vm_committed_as);

	cached = global_page_state(NR_FILE_PAGES) -
			total_swapcache_pages() - i.bufferram;
	if (cached < 0)
		cached = 0;

	get_vmalloc_info(&vmi);

	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_page_state(NR_LRU_BASE + lru);

	for_each_zone(zone)
		wmark_low += zone->watermark[WMARK_LOW];

	/*
	 * Estimate the amount of memory available for userspace allocations,
	 * without causing swapping.
	 *
	 * Free memory cannot be taken below the low watermark, before the
	 * system starts swapping.
	 */
	available = i.freeram - wmark_low;

	/*
	 * Not all the page cache can be freed, otherwise the system will
	 * start swapping. Assume at least half of the page cache, or the
	 * low watermark worth of cache, needs to stay.
	 */
	pagecache = pages[LRU_ACTIVE_FILE] + pages[LRU_INACTIVE_FILE];
	pagecache -= min(pagecache / 2, wmark_low);
	available += pagecache;

	/*
	 * Part of the reclaimable slab consists of items that are in use,
	 * and cannot be freed. Cap this estimate at the low watermark.
	 */
	available += global_page_state(NR_SLAB_RECLAIMABLE) -
		     min(global_page_state(NR_SLAB_RECLAIMABLE) / 2, wmark_low);

	if (available < 0)
		available = 0;

	/*
	 * Tagged format, for easy grepping and expansion.
	 */
	seq_printf(m,
		"MemTotal:       %8lu kB\n"//i.totalram
		"MemFree:        %8lu kB\n"//i.freeram
		"MemAvailable:   %8lu kB\n"//available
		"Buffers:        %8lu kB\n"//i.bufferram
		"Cached:         %8lu kB\n"//cached=global_page_state(NR_FILE_PAGES)-total_swapcache_pages()-i.bufferram;
		"SwapCached:     %8lu kB\n"//total_swapcache_pages()
		"Active:         %8lu kB\n"//pages[LRU_ACTIVE_ANON]   + pages[LRU_ACTIVE_FILE]
		"Inactive:       %8lu kB\n"//pages[LRU_INACTIVE_ANON] + pages[LRU_INACTIVE_FILE]
		"Active(anon):   %8lu kB\n"//pages[LRU_ACTIVE_ANON]
		"Inactive(anon): %8lu kB\n"//pages[LRU_INACTIVE_ANON]
		"Active(file):   %8lu kB\n"//pages[LRU_ACTIVE_FILE]
		"Inactive(file): %8lu kB\n"//pages[LRU_INACTIVE_FILE]
		"Unevictable:    %8lu kB\n"//pages[LRU_UNEVICTABLE]
		"Mlocked:        %8lu kB\n"//global_page_state(NR_MLOCK)
#if 1//#if 1//#ifdef CONFIG_HIGHMEM//CONFIG_HIGHMEM=y
		"HighTotal:      %8lu kB\n"
		"HighFree:       %8lu kB\n"
		"LowTotal:       %8lu kB\n"
		"LowFree:        %8lu kB\n"
#endif
#if 0//#ifndef CONFIG_MMU//CONFIG_MMU=y
		"MmapCopy:       %8lu kB\n"
#endif
		"SwapTotal:      %8lu kB\n"//i.totalswap
		"SwapFree:       %8lu kB\n"//i.freeswap
		"Dirty:          %8lu kB\n"//global_page_state(NR_FILE_DIRTY)
		"Writeback:      %8lu kB\n"//global_page_state(NR_WRITEBACK)
		"AnonPages:      %8lu kB\n"//global_page_state(NR_ANON_PAGES)
		"Mapped:         %8lu kB\n"//global_page_state(NR_FILE_MAPPED)
		"Shmem:          %8lu kB\n"
		"Slab:           %8lu kB\n"
		"SReclaimable:   %8lu kB\n"
		"SUnreclaim:     %8lu kB\n"
		"KernelStack:    %8lu kB\n"
		"PageTables:     %8lu kB\n"
#ifdef CONFIG_QUICKLIST
		"Quicklists:     %8lu kB\n"
#endif
		"NFS_Unstable:   %8lu kB\n"
		"Bounce:         %8lu kB\n"
		"WritebackTmp:   %8lu kB\n"
		"CommitLimit:    %8lu kB\n"
		"Committed_AS:   %8lu kB\n"
		"VmallocTotal:   %8lu kB\n"
		"VmallocUsed:    %8lu kB\n"
		"VmallocChunk:   %8lu kB\n"
#ifdef CONFIG_MEMORY_FAILURE
		"HardwareCorrupted: %5lu kB\n"
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		"AnonHugePages:  %8lu kB\n"
#endif
		,
		K(i.totalram),
		K(i.freeram),
		K(available),
		K(i.bufferram),
		K(cached),
		K(total_swapcache_pages()),
		K(pages[LRU_ACTIVE_ANON]   + pages[LRU_ACTIVE_FILE]),
		K(pages[LRU_INACTIVE_ANON] + pages[LRU_INACTIVE_FILE]),
		K(pages[LRU_ACTIVE_ANON]),
		K(pages[LRU_INACTIVE_ANON]),
		K(pages[LRU_ACTIVE_FILE]),
		K(pages[LRU_INACTIVE_FILE]),
		K(pages[LRU_UNEVICTABLE]),
		K(global_page_state(NR_MLOCK)),
#if 1//#if 1//#ifdef CONFIG_HIGHMEM//CONFIG_HIGHMEM=y
		K(i.totalhigh),
		K(i.freehigh),
		K(i.totalram-i.totalhigh),
		K(i.freeram-i.freehigh),
#endif
#if 0//#ifndef CONFIG_MMU//CONFIG_MMU=y
		K((unsigned long) atomic_long_read(&mmap_pages_allocated)),
#endif
		K(i.totalswap),
		K(i.freeswap),
		K(global_page_state(NR_FILE_DIRTY)),
		K(global_page_state(NR_WRITEBACK)),
		K(global_page_state(NR_ANON_PAGES)),
		K(global_page_state(NR_FILE_MAPPED)),
		K(i.sharedram),
		K(global_page_state(NR_SLAB_RECLAIMABLE) +
				global_page_state(NR_SLAB_UNRECLAIMABLE)),
		K(global_page_state(NR_SLAB_RECLAIMABLE)),
		K(global_page_state(NR_SLAB_UNRECLAIMABLE)),
		global_page_state(NR_KERNEL_STACK) * THREAD_SIZE / 1024,
		K(global_page_state(NR_PAGETABLE)),
#ifdef CONFIG_QUICKLIST
		K(quicklist_total_size()),
#endif
		K(global_page_state(NR_UNSTABLE_NFS)),
		K(global_page_state(NR_BOUNCE)),
		K(global_page_state(NR_WRITEBACK_TEMP)),
		K(vm_commit_limit()),
		K(committed),
		(unsigned long)VMALLOC_TOTAL >> 10,
		vmi.used >> 10,
		vmi.largest_chunk >> 10
#ifdef CONFIG_MEMORY_FAILURE
		,atomic_long_read(&num_poisoned_pages) << (PAGE_SHIFT - 10)
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		,K(global_page_state(NR_ANON_TRANSPARENT_HUGEPAGES) *
		   HPAGE_PMD_NR)
#endif
		);

	hugetlb_report_meminfo(m);

	arch_report_meminfo(m);

	return 0;
#undef K
}

static int meminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, meminfo_proc_show, NULL);
}

static const struct file_operations meminfo_proc_fops = {
	.open		= meminfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_meminfo_init(void)
{
	proc_create("meminfo", 0, NULL, &meminfo_proc_fops);
	return 0;
}
fs_initcall(proc_meminfo_init);