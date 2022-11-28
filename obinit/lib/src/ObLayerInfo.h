// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBLAYERINFO_H
#define OBLAYERINFO_H

#include "ob/ObDefs.h"

typedef struct ObLayerInfo
{
  char name[OB_NAME_MAX];
  char author[OB_LAYER_AUTHOR_MAX];
  char createTs[OB_TS_MAX];
  char description[OB_LAYER_DESC_MAX];
  char underlayer[OB_NAME_MAX];

  char rootPath[OB_PATH_MAX];
} ObLayerInfo;

#endif // OBLAYERINFO_H
