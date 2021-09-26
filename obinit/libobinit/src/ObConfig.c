// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

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

void obAddDurable(ObConfig* config, const char* path, bool copyOrigin)
{
  ObDurable* durable = malloc(sizeof(ObDurable));
  //TODO
  strcpy(durable->path, path);
  durable->copyOrigin = copyOrigin;
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
