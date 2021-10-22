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

#ifndef OB_CPATH_MAX
#define OB_CPATH_MAX OB_PATH_MAX*2
#endif

#ifndef OB_CCPATH_MAX
#define OB_CCPATH_MAX OB_PATH_MAX*3
#endif

#ifndef OB_DEV_PATH_MAX
#define OB_DEV_PATH_MAX 512
#endif

#ifndef OB_NAME_MAX
#define OB_NAME_MAX 255
#endif

#ifndef OB_TS_MAX
#define OB_TS_MAX 25
#endif

#ifndef OB_LAYER_AUTHOR_MAX
#define OB_LAYER_AUTHOR_MAX 32
#endif

#ifndef OB_LAYER_DESC_MAX
#define OB_LAYER_DESC_MAX 255
#endif

#ifndef OB_LAYER_ROOT_DIR
#define OB_LAYER_ROOT_DIR "/root"
#endif

#ifndef OB_LAYER_INFO_PATH
#define OB_LAYER_INFO_PATH "/etc/layer.yaml"
#endif

#ifndef OB_DEV_MOUNT_POINT
#define OB_DEV_MOUNT_POINT "/obmnt"
#endif

#ifndef OB_OVERLAY_DIR
#define OB_OVERLAY_DIR "/overlay"
#endif

#ifndef OB_USER_BINDINGS_DIR
#define OB_USER_BINDINGS_DIR "/overboot"
#endif

#ifndef OB_LAYERS_DIR_NAME
#define OB_LAYERS_DIR_NAME "layers"
#endif

#ifndef OB_DURABLES_DIR_NAME
#define OB_DURABLES_DIR_NAME "durables"
#endif

#ifndef OB_LAYER_DIR_EXT
#define OB_LAYER_DIR_EXT "obld"
#endif

#ifndef OB_UNDERLAYER_ROOT
#define OB_UNDERLAYER_ROOT "root"
#endif

#ifndef OB_UNDERLAYER_NONE
#define OB_UNDERLAYER_NONE "none"
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

#ifndef OB_TMPFS_BLOCK_OPTIONS
#define OB_TMPFS_BLOCK_OPTIONS "size=256,mode=0600"
#endif

#ifndef OB_TMPFS_BLOCK_INFO_FILE
#define OB_TMPFS_BLOCK_INFO_FILE "README.txt"
#endif

#ifndef OB_TMPFS_BLOCK_INFO_TEXT
#define OB_TMPFS_BLOCK_INFO_TEXT "This directory has been hidden by Overboot"
#endif

#endif // OBDEFS_H
