// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBDEFS_H
#define OBDEFS_H

#ifndef OB_PREFIX_MAX
# define OB_PREFIX_MAX 256
#endif

#ifndef OB_PATH_MAX
#define OB_PATH_MAX 512
#endif

#ifndef OB_DEV_PATH_MAX
#define OB_DEV_PATH_MAX 512
#endif

#ifndef OB_NAME_MAX
#define OB_NAME_MAX 255
#endif

#ifndef OB_DEV_MOUNT_POINT
#define OB_DEV_MOUNT_POINT "/obmnt"
#endif

#ifndef OB_OVERLAY_DIR
#define OB_OVERLAY_DIR "/overlay"
#endif

#ifndef OB_DEV_MOUNT_MODE
#define OB_DEV_MOUNT_MODE 0700
#endif

#ifndef OB_MKPATH_MODE
#define OB_MKPATH_MODE 0700
#endif

#ifndef OB_DEV_IMAGE_FS
#define OB_DEV_IMAGE_FS "ext4"
#endif

#ifndef OB_DEV_MOUNT_FLAGS
#define OB_DEV_MOUNT_FLAGS 0
#endif

#ifndef OB_DEV_MOUNT_OPTIONS
#define OB_DEV_MOUNT_OPTIONS ""
#endif

#endif // OBDEFS_H
