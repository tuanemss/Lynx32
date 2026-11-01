/*
 *  ibex - nand-related stuff (Block device abstraction)
 *
 *  Copyright (c) 2015 xerub
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "plib.h"
#include "nand.h"

int bdev_read(void *bdev, void *buf, long long offset, long long size)
{
    if (version < 1940) {
        return ((struct bdev6_t *)bdev)->bdev_read(bdev, buf, offset, size);
    }
    return ((struct bdev_t *)bdev)->bdev_read(bdev, buf, offset, size);
}

const char *bdev_name(void *bdev)
{
    if (version < 1940) {
        return ((struct bdev6_t *)bdev)->name;
    }
    return ((struct bdev_t *)bdev)->name;
}
