
#ifndef __MKFSTAB_H__
#define __MKFSTAB_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#define TARGET "/target"
#define MAX_ENTRIES 256
#define HEADER \
	"# %s: static file system information.\n" \
	"#\n" \
	"# <file system> <mount point>   <type>  <options>       <dump>  <pass>\n"

#ifdef LOCAL
#define FSTAB_FILE "/tmp/fstab"
#define FSTAB_D "/tmp/fstab.d"
#define MAPDEVFS "../../utils/mapdevfs"
#else
#define FSTAB_FILE "/target/etc/fstab"
#define FSTAB_D "/var/lib/partconf/fstab.d"
#define MAPDEVFS "/usr/bin/mapdevfs"
#endif

struct fstab_entry {
	char *filesystem;
	char *mountpoint;
	char *typ;
	char *options;
	int dump;
	int pass;
};

void insert_line(const char *line);
void get_fstab_d_dir();
void get_filesystems();
void get_swapspaces();
int main(int argc, char *argv[]);

#endif /* __MKFSTAB_H__ */

