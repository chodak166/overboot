// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ob/ObConfig.h"

#include <stdlib.h>

#include <string.h>

static void obCountDurablesRecursive(ObDurable* durable, int* count)
{
  if (durable != NULL) {
    *count += 1;
    obCountDurablesRecursive(durable->next, count);
  }
}

// --------- public API ---------- //

void obAddDurable(ObConfig* config, const char* path)
{
  ObDurable* durable = malloc(sizeof(ObDurable));
  strcpy(durable->path, path);
  durable->copyOrigin = false;
  durable->forceFileType = false;
  durable->next = config->durable;
  config->durable = durable;
}

int obCountDurables(const ObConfig* config)
{
  int count = 0;
  obCountDurablesRecursive(config->durable, &count);
  return count;
}

void obFreeDurable(ObDurable* durable)
{
  if (durable == NULL) {
    return;
  }

  if (durable->next != NULL) {
    obFreeDurable(durable->next);
  }
  free(durable);
}
