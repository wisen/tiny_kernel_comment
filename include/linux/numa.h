#ifndef _LINUX_NUMA_H
#define _LINUX_NUMA_H


#ifdef CONFIG_NODES_SHIFT
#define NODES_SHIFT     CONFIG_NODES_SHIFT
#else
#define NODES_SHIFT     0
#endif

#define MAX_NUMNODES    (1 << NODES_SHIFT)

#define	NUMA_NO_NODE	(-1)
//wisen: ARM Linux采用了UMA方式的内存管理机制.UMA对应一致存储结构，
//wisen: 它只需要一个Node就可以描述当前系统中的物理内存，但是NUMA的
//wisen: 出现打破了这种平静，此时需要多个Node，它们被统一定义为一个
//wisen: 名为discontig_node_data的数组。为了和UMA兼容，就将描述UMA存储
//wisen: 结构的描述符contig_page_data放到该数组的第一个元素中。
//wisen: 内核配置选项CONFIG_NUMA决定了当前系统是否支持NUMA机制。此时
//wisen: 无论UMA还是NUMA，它们都是对应到一个类型为pg_data_t的数组中，便于统一管理。
//wisen: Linux把物理内存划分为三个层次来管理：存储节点（Node）、管理区（Zone）
//wisen: 和页面（Page）.一个UMA系统中只有一个Node，而在NUMA中则可以存在多个Node。
//wisen: 它由CONFIG_NODES_SHIFT配置选项决定，它是CONFIG_NUMA的子选项，所以只有
//wisen: 配置了CONFIG_NUMA，该选项才起作用。UMA情况下，NODES_SHIFT被定义为0，
//wisen: MAX_NUMNODES也即为1。
#endif /* _LINUX_NUMA_H */
