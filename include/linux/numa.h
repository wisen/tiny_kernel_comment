#ifndef _LINUX_NUMA_H
#define _LINUX_NUMA_H


#ifdef CONFIG_NODES_SHIFT
#define NODES_SHIFT     CONFIG_NODES_SHIFT
#else
#define NODES_SHIFT     0
#endif

#define MAX_NUMNODES    (1 << NODES_SHIFT)

#define	NUMA_NO_NODE	(-1)
//wisen: ARM Linux������UMA��ʽ���ڴ�������.UMA��Ӧһ�´洢�ṹ��
//wisen: ��ֻ��Ҫһ��Node�Ϳ���������ǰϵͳ�е������ڴ棬����NUMA��
//wisen: ���ִ���������ƽ������ʱ��Ҫ���Node�����Ǳ�ͳһ����Ϊһ��
//wisen: ��Ϊdiscontig_node_data�����顣Ϊ�˺�UMA���ݣ��ͽ�����UMA�洢
//wisen: �ṹ��������contig_page_data�ŵ�������ĵ�һ��Ԫ���С�
//wisen: �ں�����ѡ��CONFIG_NUMA�����˵�ǰϵͳ�Ƿ�֧��NUMA���ơ���ʱ
//wisen: ����UMA����NUMA�����Ƕ��Ƕ�Ӧ��һ������Ϊpg_data_t�������У�����ͳһ����
//wisen: Linux�������ڴ滮��Ϊ��������������洢�ڵ㣨Node������������Zone��
//wisen: ��ҳ�棨Page��.һ��UMAϵͳ��ֻ��һ��Node������NUMA������Դ��ڶ��Node��
//wisen: ����CONFIG_NODES_SHIFT����ѡ�����������CONFIG_NUMA����ѡ�����ֻ��
//wisen: ������CONFIG_NUMA����ѡ��������á�UMA����£�NODES_SHIFT������Ϊ0��
//wisen: MAX_NUMNODESҲ��Ϊ1��
#endif /* _LINUX_NUMA_H */
