/* 

   distribute.c - Part of autopartkit, a module to partition devices
                  for debian-installer.

   Author - Petter Reinholdtsen

   Copyright (C) 2002  Petter Reinholdtsen <pere@hungry.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


 The partition distribution algorithm:

 1. For all requested partitions, place the partition on the first
    disk with enough free space to fullfill the minimum size
    requirement.  Reduce the amount of free space available on the
    disk for each partition added to the disk.

 2. Do the following for each disk:

   1. Calculate the sum of all additionally requrested diskspace
      (max-min).  Each disk with no limit to the maximum size
      get maximum equal to the disk capasity to make sure this
      partition will get the whole disk if there are no other
      partitions requesting more diskspace on this disk, and that
      the other partitions will get a share of the disk.  The other
      partitions would not get any share if unlimited request was
      treated as an infinite number.

   2. For each partition on the disk, calculate the new size by first
      calculating how much more diskspace is requested for the given
      partition (max-min), and then multiplying this size with the
      free space on the disk divided by the sum of all requested
      extra space.  This will give how much of the available free
      space this partition can get if there is less free space then
      requested space.  If there is more free space than requested
      size, the requested extra space will be bigger then the
      maximum size, and must be truncated.  The new size is set to
      the current size plus the allocated extra space, and the
      amount of free space on the disk is reduced by the extra space
      allocated.  The sum of requested diskspace for this disk is
      reduced by (max-min) to make sure the free space number is
      comparable to the requested diskspace.

This code is not finished yet, as it must be integrated into
autopartkit.

*/

#include <stdio.h>
#include <stdlib.h>
#include "autopartkit.h"
#include <assert.h>

#include "parted-compat.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* 
 * Find and return the first disk with size amount of free space.
 */
static struct disk_info_t *
find_disk_with_freespace(struct disk_info_t diskinfo[], PedSector blocks)
{
    int i;
    for (i = 0; diskinfo[i].capasity; i++)
    {
	if (diskinfo[i].freespace >= blocks)
	    return &diskinfo[i];
    }
    return NULL;
}

/*
 * Decide how to distribute the requested partitions on the available
 * disks.
 */
int
distribute_partitions(struct disk_info_t diskinfo[],
		      struct diskspace_req_s reqs[])
{
    int i;
    PedSector maxmax_blk = 0;

    /* Initialize reqs and convert MiB to blocks. */
    for (i = 0; reqs[i].mountpoint; ++i)
    {
	reqs[i].min_blk = MiB_TO_BLOCKS(reqs[i].minsize);
	if (-1 == reqs[i].maxsize)
	    reqs[i].max_blk = (PedSector)-1;
	else
	    reqs[i].max_blk = MiB_TO_BLOCKS(reqs[i].maxsize);
	reqs[i].blocks  = 0;
	reqs[i].curdisk = NULL;
    }

    /* Distribute the partiontions using minimum sizes */
    for (i = 0; reqs[i].mountpoint; ++i)
    {
	struct disk_info_t *disk;
	if (0 == reqs[i].max_blk)
	    continue; /* Ignore zero-size partitions */
	disk = find_disk_with_freespace(diskinfo, reqs[i].min_blk);
	if (NULL == disk)
	{
	    /* Unable to find disk with enough free space */
	    autopartkit_error(0, "Unable to find %llu blocks of "
			      "free space requrested by %s on any disk.\n",
			      reqs[i].min_blk,
			      reqs[i].mountpoint);
	    return -1;
	}
	else
	{
            maxmax_blk = MAX(maxmax_blk, reqs[i].max_blk);
            maxmax_blk = MAX(maxmax_blk, disk->capasity);

	    /* Found a usable disk.  Add this partition to the disk. */
	    reqs[i].blocks  = reqs[i].min_blk;
	    reqs[i].curdisk = disk;
	    disk->freespace -= reqs[i].min_blk;
	}
    }

    /* Grow the partitions as much as possible on the allocated disk. */
    for (i = 0; diskinfo[i].capasity; i++)
    {
	int j;
	long long total_wanted = 0;
	/* First find out how much extra space the partitions on this disk
	   want, ie the sum of all max_blk-min_blk */
	for (j = 0; reqs[j].mountpoint; ++j)
	{
	    if (0 == reqs[j].max_blk)
	        continue; /* Ignore zero-size partitions */
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
		if ((PedSector)-1 == reqs[j].max_blk)
		    reqs[j].max_blk = maxmax_blk;
		total_wanted += reqs[j].max_blk - reqs[j].min_blk;
	    }
	}
	/* This is where we decide the new size of the partitions */
	for (j = 0; reqs[j].mountpoint; ++j)
        {
	    if (0 == reqs[j].max_blk)
	        continue; /* Ignore zero-size partitions */
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
		PedSector newsize;
		/* These calculations can overflow if the numbers are too big. */
		newsize = reqs[j].max_blk - reqs[j].min_blk;
		newsize *= diskinfo[i].freespace;
		newsize /= total_wanted;
		newsize += reqs[j].blocks;
		if (newsize > reqs[j].max_blk)
		    newsize = reqs[j].max_blk;

		/* We know the new size.  Activate it */
		diskinfo[i].freespace -= newsize - reqs[j].blocks;
		total_wanted -= reqs[j].max_blk - reqs[j].min_blk;
		reqs[j].blocks = newsize;
	    }
	}
    }
    return 0;
}

void
print_list(struct disk_info_t diskinfo[], struct diskspace_req_s reqs[])
{
    int i;
    autopartkit_log(0, "Suggested partition distribution:\n");
    for (i = 0; diskinfo[i].capasity; i++)
    {
        int j;
	autopartkit_log(0, "  free block '%s' [%llu,%llu]:\n",
	       diskinfo[i].path ? diskinfo[i].path : "[null]",
	       BLOCKS_TO_MiB(diskinfo[i].capasity),
	       BLOCKS_TO_MiB(diskinfo[i].freespace));
	for (j = 0; reqs[j].mountpoint; ++j)
	{
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
		autopartkit_log(0,
				"    %s [%llu <= %llu <= %llu] [%lld <= %lld]"
				" %s %lld-%lld\n",
				( reqs[j].mountpoint ?
				  reqs[j].mountpoint : "[null]" ),
				BLOCKS_TO_MiB(reqs[j].min_blk),
				BLOCKS_TO_MiB(reqs[j].blocks),
				BLOCKS_TO_MiB(reqs[j].max_blk),
				(unsigned long long)reqs[j].minsize,
				(unsigned long long)reqs[j].maxsize,
				reqs[j].curdisk->path,
				reqs[j].curdisk->geom.start,
				reqs[j].curdisk->geom.end);
	    }
	}
    }
    autopartkit_log(1, "Done listing suggested partition distribution.\n");
}

struct disk_info_t *
get_free_space_list(void)
{
    PedDevice *dev = NULL;
    PedDisk *disk = NULL;
    PedPartition *part = NULL;

    int spacenum = 0;
    const int spacecapasity = 10;

    struct disk_info_t *spaceinfo = calloc(sizeof(*spaceinfo), spacecapasity);

    autopartkit_log(2, "Locating free space on all disks\n");

    /* Detect all devices */
    ped_device_probe_all();

    /* Loop over the detected devices */
    for (dev = ped_device_get_next(NULL); dev; dev = ped_device_get_next(dev))
    {
        assert(dev);

	autopartkit_log(2, "  checking dev: %s, sector_size=%d\n",
			dev->path,
			dev->sector_size /* in bytes */);

        disk = ped_disk_new(dev);

	for(part = ped_disk_next_partition(disk, NULL); part;
	    part = ped_disk_next_partition(disk, part))
	{
	    assert(part);

	    autopartkit_log( 2,
			     "    part: %d, type: %d size: (%lld-%lld) %lld\n",
			     part->num, part->type,
			     part->geom.start,part->geom.end, 
			     part->geom.length);

	    if ((part->type & PED_PARTITION_FREESPACE) ==
		PED_PARTITION_FREESPACE)
	    {
	        autopartkit_log(2, "    free space %lld\n", part->geom.length);
		/* Store the info we need to find this block again */
		spaceinfo[spacenum].path = disk->dev->path;
		memcpy(&spaceinfo[spacenum].geom, &part->geom,
		       sizeof(part->geom));
		spaceinfo[spacenum].capasity = part->geom.length;
		spaceinfo[spacenum].freespace = part->geom.length;
		++spacenum;
		assert(spacenum < spacecapasity);
	    }
	}
    }
    autopartkit_log(2, "Done locating free space, found %d free areas\n",
		    spacenum);
    if (0 < spacenum)
        return spaceinfo;
    else
        return NULL;
}
