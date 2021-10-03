// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObFstab.h"
#include "ob/ObDefs.h"
#include "ob/ObLogging.h"
#include "sds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static bool isWhiteChar(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == EOF;
}

static int getNthColumn(char* buffer, const char* line, ssize_t size, int n)
{
  int8_t column = -1;
  int16_t pos = 0;
  int16_t initPos = -1;

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

static bool getRootMtabEntry(char* entryBuffer,
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
    return false;
  }

  while ((read = getline(&line, &len, fp)) != -1) {
    getNthColumn(entryBuffer, line, read, 1);
    if (strcmp(rootPath, entryBuffer) == 0) {
      found = true;
      strcpy(entryBuffer, line);
      break;
    }
  }

  fclose(fp);
  if (line) {
    free(line);
  }
  return found;
}

static void rewriteFstab(FILE* origFile, FILE* file, const char* mtabEntry)
{
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  char columnBuffer[OB_PATH_MAX] = {0};
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
    else if (read > 0) {
      fputs(line, file);
    }
  }

  if (line) {
    free(line);
  }
}

static bool updateFstabWithMtabEntry(const char* fstabPath,
                             const char* mtabEntry)
{
  sds fstabPathOrig = sdsnew(fstabPath);
  fstabPathOrig = sdscat(fstabPathOrig, ".orig");
  rename(fstabPath, fstabPathOrig);

  FILE* origFile = NULL;
  FILE* file = NULL;

  origFile = fopen(fstabPathOrig, "r");
  if (origFile == NULL) {
    obLogE("Cannot open %s: %s", fstabPathOrig, strerror(errno));
    sdsfree(fstabPathOrig);
    return false;
  }

  file = fopen(fstabPath, "w");
  if (file == NULL) {
    obLogE("Cannot open %s: %s", fstabPath, strerror(errno));
    sdsfree(fstabPathOrig);
    return false;
  }
  sdsfree(fstabPathOrig);

  rewriteFstab(origFile, file, mtabEntry);

  fclose(origFile);
  fclose(file);

  return true;
}


// --------- public API ---------- //

bool obUpdateFstab(const char* rootmnt, const char* mtabPath)
{
  sds fstabPath = sdsnew(rootmnt);
  fstabPath = sdscat(fstabPath, "/etc/fstab");

  obLogI("Updating fstab (%s)", fstabPath);

  bool result = false;
  char rootMtabEntry[OB_PATH_MAX] = {0};
  if (getRootMtabEntry(rootMtabEntry, mtabPath, rootmnt)) {
    result = updateFstabWithMtabEntry(fstabPath, rootMtabEntry);
  }
  else {
    obLogW("Root entry (%s) not found in %s", rootMtabEntry, mtabPath);
  }

  sdsfree(fstabPath);
  return result;
}

