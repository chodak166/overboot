// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObLayerCollector.h"
#include "ObLayerInfo.h"
#include "ObYamlLayerReader.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static bool isRootLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_ROOT, layerName) == 0
      || strcmp("", layerName) == 0;
}

static bool isEndLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_NONE, layerName) == 0;
}


// --------- public API ---------- //


ObLayerItem* obCollectLayers(const char* layersDir, const char* layerName,
                             const char* lowerPath, uint8_t* count)
{
  ObLayerItem* item = NULL;

  if (isRootLayer(layerName)) {
    item = calloc(1, sizeof(ObLayerItem));
    strcpy(item->layerPath, lowerPath);
    *count += 1;
  }
  else if (!isEndLayer(layerName)) {
    ObLayerInfo info;
    if (obLoadLayerInfo(layersDir, layerName, &info)) {
      item = calloc(1, sizeof(ObLayerItem));
      strcpy(item->layerPath, info.rootPath);
      item->prev = obCollectLayers(layersDir, info.underlayer, lowerPath, count);
      *count += 1;
    }
  }
  return item;
}
