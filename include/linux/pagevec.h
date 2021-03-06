/*
 * include/linux/pagevec.h
 *
 * In many places it is efficient to batch an operation up against multiple
 * pages.  A pagevec is a multipage container which is used for that.
 */

#ifndef _LINUX_PAGEVEC_H
#define _LINUX_PAGEVEC_H

/* 14 pointers + two long's align the pagevec structure to a power of two */
#define PAGEVEC_SIZE	14

struct page;
struct address_space;

//LRU 缓存
//页面根据其活跃程度会在 active 链表和 inactive链表之间来回移动，如果要将某个页面插入到这两个链表中去
//必须要通过自旋锁(锁的动作在pagevec_lru_move_fn中)以保证对链表的并发访问操作不会出错。
//为了降低锁的竞争，Linux提供了一种特殊的缓存：
//LRU 缓存，用以批量地向 LRU 链表中快速地添加页面。有了 LRU缓存之后，新页不会被马上添加到相应的
//链表上去，而是先被放到一个缓冲区中去，当该缓冲区缓存了足够多的页面之后，
//缓冲区中的页面才会被一次性地全部添加到相应的LRU 链表中去。
//Linux采用这种方法降低了锁的竞争，极大地提升了系统的性能。

/*
 用来实现 LRU 缓存的两个关键函数是 lru_cache_add() 和 lru_cache_add_active()。前者用于延迟将页面添加到 inactive 链表上去，后者用于延迟将页面添加到 active 链表上去。这两个函数都会将要移动的页面先放到页向量 pagevec 中，当 pagevec满了(已经装了 14个页面的描述符指针)，pagevec 结构中的所有页面才会被一次性地移动到相应的链表上去。
 */
struct pagevec {
	unsigned long nr;
	unsigned long cold;
	struct page *pages[PAGEVEC_SIZE];
};

void __pagevec_release(struct pagevec *pvec);
void __pagevec_lru_add(struct pagevec *pvec);
unsigned pagevec_lookup_entries(struct pagevec *pvec,
				struct address_space *mapping,
				pgoff_t start, unsigned nr_entries,
				pgoff_t *indices);
void pagevec_remove_exceptionals(struct pagevec *pvec);
unsigned pagevec_lookup(struct pagevec *pvec, struct address_space *mapping,
		pgoff_t start, unsigned nr_pages);
unsigned pagevec_lookup_tag(struct pagevec *pvec,
		struct address_space *mapping, pgoff_t *index, int tag,
		unsigned nr_pages);

static inline void pagevec_init(struct pagevec *pvec, int cold)
{
	pvec->nr = 0;
	pvec->cold = cold;
}

static inline void pagevec_reinit(struct pagevec *pvec)
{
	pvec->nr = 0;
}

static inline unsigned pagevec_count(struct pagevec *pvec)
{
	return pvec->nr;
}

static inline unsigned pagevec_space(struct pagevec *pvec)
{
	return PAGEVEC_SIZE - pvec->nr;
}

/*
 * Add a page to a pagevec.  Returns the number of slots still available.
 */
static inline unsigned pagevec_add(struct pagevec *pvec, struct page *page)
{
	pvec->pages[pvec->nr++] = page;
	return pagevec_space(pvec);
}

static inline void pagevec_release(struct pagevec *pvec)
{
	if (pagevec_count(pvec))
		__pagevec_release(pvec);
}

#endif /* _LINUX_PAGEVEC_H */
