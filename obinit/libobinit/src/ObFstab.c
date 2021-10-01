// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObFstab.h"
#include "ob/ObDefs.h"
#include "ob/ObLogging.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static bool isWhiteChar(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == EOF;
}

static int getNthColumn(char* buffer, const char* line, ssize_t size, int n)
{
  int column = -1;
  int pos = 0;
  int initPos = -1;

  while (pos < size && column < n) {
    while (isWhiteChar(line[pos])) {
      pos += 1;
    }

    if (initPos == pos) {
      while (!isWhiteChar(line[pos])) {
        pos +=1;
      }
    }
    else {
      column += 1;
    }

    initPos = pos;
  }

  int endPos = pos;
  while (!isWhiteChar(line[endPos])) {
    endPos +=1;
  }

  memcpy(buffer, &line[pos], endPos - pos + 1);
  buffer[endPos-pos] = '\0';

  return pos;
}

static int getRootMtabEntry(char* entryBuffer,
                     const char* mtabPath,
                     const char* rootPath)
{
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  bool found = false;

  fp = fopen(mtabPath, "r");
  if (fp == NULL) {
    obLogE("Cannot open %s: %s", mtabPath, strerror(errno));
    return 1;
  }

  while ((read = getline(&line, &len, fp)) != -1) {
    getNthColumn(entryBuffer, line, read, 1);
    if (strcmp(rootPath, entryBuffer) == 0) {
      found = true;
      strcpy(entryBuffer, line);
      printf("Matched root entry:\n%s\n", entryBuffer);
      break;
    }
  }

  fclose(fp);
  if (line) {
    free(line);
  }

  return found ? 0 : 1;
}

static int updateFstabWithMtabEntry(const char* fstabPath,
                             const char* mtabEntry)
{
  char fstabPathOrig[OB_PATH_MAX];
  sprintf(fstabPathOrig, "%s.orig", fstabPath);
  rename(fstabPath, fstabPathOrig);

  FILE* origFile = NULL;
  FILE* file = NULL;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  origFile = fopen(fstabPathOrig, "r");
  if (origFile == NULL) {
    obLogE("Cannot open %s: %s", fstabPathOrig, strerror(errno));
    return 1;
  }

  file = fopen(fstabPath, "w");
  if (file == NULL) {
    obLogE("Cannot open %s: %s", fstabPath, strerror(errno));
    return 1;
  }

  char columnBuffer[OB_PATH_MAX];
  while ((read = getline(&line, &len, origFile)) != -1) {
    getNthColumn(columnBuffer, line, read, 1);
    if (strcmp(columnBuffer, "/") == 0) {
      obLogI("Root entry found in orig fstab: %s", line);
      int columnPos = getNthColumn(columnBuffer, mtabEntry, read, 1);
      char entry[OB_PATH_MAX];
      strcpy(entry, mtabEntry);
      entry[columnPos] = '\0';
      fputs(entry, file);
      fputs("/", file);
      fputs(entry + columnPos + strlen(columnBuffer), file);
    }
    else {
      fputs(line, file);
    }

  }

  fclose(origFile);
  fclose(file);
  if (line) {
    free(line);
  }

  return 0;
}


// --------- public API ---------- //

int updateFstab(const char* rootmnt, const char* mtabPath)
{
  char fstabPath[OB_PATH_MAX];
  sprintf(fstabPath, "%s/etc/fstab", rootmnt);

  obLogI("Updating fstab (%s)", fstabPath);

  //TODO: error handling
  char rootMtabEntry[OB_PATH_MAX];
  getRootMtabEntry(rootMtabEntry, mtabPath, rootmnt);
  updateFstabWithMtabEntry(fstabPath, rootMtabEntry);

  return 0;
}

