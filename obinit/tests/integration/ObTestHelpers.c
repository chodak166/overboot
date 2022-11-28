// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObTestHelpers.h"
#include "ob/ObDefs.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

bool obConcatPaths(char* result, const char* pathA, const char* pathB)
{
  if (strlen(pathA) + strlen(pathB) > OB_PATH_MAX) {
    fprintf(stderr, "Cannot concat %s and %s: result path too long\n", pathA, pathB);
    return false;
  }
  strcpy(result, pathA);
  strcat(result, pathB);
  return true;
}

char* obGetSelfPath(char* buffer, int size)
{
  int result = readlink("/proc/self/exe", buffer, size - 1);
  if (result < 0 || (result >= size - 1)) {
    return NULL;
  }

  buffer[result] = '\0';
  for (int i = result; i >= 0; i--) {
    if (buffer[i] == '/') {
      buffer[i] = '\0';
      break;
    }
  }
  return buffer;
}

char* obReadFile(const char* path, char* content)
{
  FILE* file = fopen(path, "r");
  if (file != NULL) {
    fscanf(file, "%s", content);
    fclose(file);
  }
  else {
    content[0] = '\0';
  }
  return content;
}

void obCreateFile(const char* path, const char* content)
{
  FILE* file = fopen(path, "w");
  if (file != NULL) {
    fputs(content, file);
    fclose(file);
  }
}
