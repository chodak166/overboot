// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBLAYERCOLLECTOR_H
#define OBLAYERCOLLECTOR_H

#include "ob/ObDefs.h"
#include <inttypes.h>

struct ObLayerItem;
typedef struct ObLayerItem
{
  char layerPath[OB_PATH_MAX];
  struct ObLayerItem* prev;
} ObLayerItem;

ObLayerItem* obCollectLayers(const char* layersDir, const char* layerName,
                             const char* lowerPath, uint8_t* count);


#endif // OBLAYERCOLLECTOR_H
