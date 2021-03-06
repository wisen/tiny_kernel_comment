#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

#include <linux/dcache.h>
#include <linux/errno.h>
#include <linux/linkage.h>
#include <linux/path.h>

struct vfsmount;

enum { MAX_NESTED_LINKS = 8 };

struct nameidata {
	struct path	path;//保存当前搜索到的路径
	struct qstr	last;//保存当前子路径名及其散列值；
	struct path	root;//用来保存根目录的信息；
	struct inode	*inode; /* path.dentry.d_inode *///指向当前找到的目录项的 inode 结构；
	unsigned int	flags;
	unsigned	seq, m_seq;//seq 是相关目录项的顺序锁序号；是相关文件系统（其实是 mount）的顺序锁序号；
	int		last_type;//表示当前节点类型；
	unsigned	depth;//解析符号链接过程中的递归深度
	char *saved_names[MAX_NESTED_LINKS + 1];//用来记录相应递归深度的符号链接的路径。
};

/*
 * Type of the last component on LOOKUP_PARENT
 */
////分别代表了普通文件，根文件，.文件，..文件和符号链接文件
enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT, LAST_BIND};

/*
 * The bitmask for a lookup event:
 *  - follow links at the end
 *  - require a directory
 *  - ending slashes ok even for nonexistent files
 *  - internal "there are more path components" flag
 *  - dentry cache is untrusted; force a real lookup
 *  - suppress terminal automount
 */
#define LOOKUP_FOLLOW		0x0001
#define LOOKUP_DIRECTORY	0x0002
#define LOOKUP_AUTOMOUNT	0x0004

#define LOOKUP_PARENT		0x0010
#define LOOKUP_REVAL		0x0020
#define LOOKUP_RCU		0x0040

/*
 * Intent data
 */
#define LOOKUP_OPEN		0x0100
#define LOOKUP_CREATE		0x0200
#define LOOKUP_EXCL		0x0400
#define LOOKUP_RENAME_TARGET	0x0800

#define LOOKUP_JUMPED		0x1000
#define LOOKUP_ROOT		0x2000
#define LOOKUP_EMPTY		0x4000

extern int user_path_at(int, const char __user *, unsigned, struct path *);
extern int user_path_at_empty(int, const char __user *, unsigned, struct path *, int *empty);

#define user_path(name, path) user_path_at(AT_FDCWD, name, LOOKUP_FOLLOW, path)
#define user_lpath(name, path) user_path_at(AT_FDCWD, name, 0, path)
#define user_path_dir(name, path) \
	user_path_at(AT_FDCWD, name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, path)

extern int kern_path(const char *, unsigned, struct path *);

extern struct dentry *kern_path_create(int, const char *, struct path *, unsigned int);
extern struct dentry *user_path_create(int, const char __user *, struct path *, unsigned int);
extern void done_path_create(struct path *, struct dentry *);
extern struct dentry *kern_path_locked(const char *, struct path *);
extern int kern_path_mountpoint(int, const char *, struct path *, unsigned int);
extern int vfs_path_lookup(struct dentry *, struct vfsmount *,
		const char *, unsigned int, struct path *);

extern struct dentry *lookup_one_len(const char *, struct dentry *, int);

extern int follow_down_one(struct path *);
extern int follow_down(struct path *);
extern int follow_up(struct path *);

extern struct dentry *lock_rename(struct dentry *, struct dentry *);
extern void unlock_rename(struct dentry *, struct dentry *);

extern void nd_jump_link(struct nameidata *nd, struct path *path);

static inline void nd_set_link(struct nameidata *nd, char *path)
{
	nd->saved_names[nd->depth] = path;
}

static inline char *nd_get_link(struct nameidata *nd)
{
	return nd->saved_names[nd->depth];
}

static inline void nd_terminate_link(void *name, size_t len, size_t maxlen)
{
	((char *) name)[min(len, maxlen)] = '\0';
}

/**
 * retry_estale - determine whether the caller should retry an operation
 * @error: the error that would currently be returned
 * @flags: flags being used for next lookup attempt
 *
 * Check to see if the error code was -ESTALE, and then determine whether
 * to retry the call based on whether "flags" already has LOOKUP_REVAL set.
 *
 * Returns true if the caller should try the operation again.
 */
static inline bool
retry_estale(const long error, const unsigned int flags)
{
	return error == -ESTALE && !(flags & LOOKUP_REVAL);
}

#endif /* _LINUX_NAMEI_H */
