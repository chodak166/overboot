// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ob/ObMount.h"
#include "ob/ObOsUtils.h"
#include "ObYamlConfigReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef OB_LOG_STDOUT
# define OB_LOG_USE_STD true
#else
# define OB_LOG_USE_STD false
#endif


#ifdef OB_LOG_KMSG
# define OB_LOG_USE_KMSG true
#else
# define OB_LOG_USE_KMSG false
#endif

bool isWhiteChar(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == EOF;
}

int getNthColumn(char* buffer, const char* line, ssize_t size, int n)
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

int getRootMtabEntry(char* entryBuffer,
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
  if (line)
    free(line);

  return found ? 0 : 1;
}

int updateFstabWithMtabEntry(const char* fstabPath,
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

int main(int argc, char* argv[])
{
  obInitLogger(OB_LOG_USE_STD, OB_LOG_USE_KMSG);

  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  ObContext* context = obCreateObContext(options.rootPrefix);

  obLoadYamlConfig(context, options.configFile);

  obLogI("enabled: %i", context->enabled);
  obLogI("use tmpfs: %i", context->useTmpfs);
  obLogI("tmpfs size: %s", context->tmpfsSize);
  obLogI("bind layers: %i", context->bindLayers);
  obLogI("Device path: %s", context->devicePath);
  obLogI("head layer: %s", context->headLayer);
  obLogI("repo: %s", context->repository);

  int durablesCount = obCountDurables(context);

  obLogI("durables (%i):", durablesCount);

  ObDurable* durable = context->durable;
  while (durable != NULL) {
    obLogI("path: %s, copy_origin: %i", durable->path, durable->copyOrigin);
    durable = durable->next;
  }

  if (!obFindDevice(context)) {
    obLogE("Device %s not found", context->devicePath);
    return EXIT_FAILURE;
  }

  if (!obMountDevice(context)) {
    obLogE("Device mount (%s -> %s) failed", context->devicePath, context->devMountPoint);
    return EXIT_FAILURE;
  }

  \
  if (!obPrepareOverlay(context)) {
    obLogE("Cannot prepare overlay dir (%s)", context->overlayDir);
    return EXIT_FAILURE;
  }

  char upperPath[OB_DEV_PATH_MAX];
  sprintf(upperPath, "%s/upper", context->overlayDir);

  if (!context->useTmpfs) {
    char srcPath[OB_PATH_MAX];
    sprintf(srcPath, "%s/%s/upper", context->devMountPoint, context->repository);

    if (context->clearUpper) {
      obRemoveDirR(srcPath);
      obMkpath(srcPath, OB_MKPATH_MODE);
    }
    else if (!obExists(srcPath)) {
      obMkpath(srcPath, OB_MKPATH_MODE);
    }

    obRbind(srcPath, upperPath);
  }

  char rootmntPath[OB_PATH_MAX];
  char lowerPath[OB_PATH_MAX];

  char* rootmnt = getenv("rootmnt");
  if (rootmnt != NULL) {
    sprintf(rootmntPath, "%s%s", context->prefix, rootmnt);
  }
  else {
    sprintf(rootmntPath, "%s/root", context->prefix);
    obLogI("The rootmnt environment variable not set, using %s", rootmntPath);
  }
  sprintf(lowerPath, "%s/lower", context->overlayDir);
  obMove(rootmntPath, lowerPath);

  char workPath[OB_DEV_PATH_MAX];
  sprintf(workPath, "%s/work", context->overlayDir);

  char repoPath[OB_PATH_MAX];
  sprintf(repoPath, "%s/%s", context->devMountPoint, context->repository);

  //TODO colect layers
  char** layers = malloc(1 * sizeof(char*));
  layers[0] = malloc(OB_PATH_MAX);
  strcpy(layers[0], lowerPath);
  if (!obMountOverlay(layers, 1, upperPath, workPath, rootmntPath)) {
    obLogE("Cannot mount overlay");
  }

  free(layers[0]);
  free(layers);
  //TODO remove layers[][]

  char bindedOverlay[OB_PATH_MAX];
  sprintf(bindedOverlay, "%s/overlay", rootmntPath);
  obRbind(context->overlayDir, bindedOverlay);

  if (context->bindLayers) {
    char bindedRepo[OB_PATH_MAX];
    sprintf(bindedRepo, "%s/layers", bindedOverlay);
    obMkpath(bindedRepo, OB_MKPATH_MODE);
    obRbind(repoPath, bindedRepo);
  }

  char mtabPath[OB_PATH_MAX];
  sprintf(mtabPath, "%s/etc/mtab", context->prefix);

  updateFstab(rootmntPath, mtabPath);

  obFreeObContext(&context);
  return 0;
}
