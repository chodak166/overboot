// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ob/ObMount.h"
#include "ob/ObOsUtils.h"
#include "ob/ObYamlConfigReader.h"
#include "ob/ObYamlLayerReader.h"

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

struct ObLayerItem;
typedef struct ObLayerItem
{
  char layerPath[OB_PATH_MAX];
  struct ObLayerItem* prev;
} ObLayerItem;

bool isRootLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_ROOT, layerName) == 0
      || strcmp("", layerName) == 0;
}

bool isEndLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_NONE, layerName) == 0;
}

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

int main(int argc, char* argv[])
{
  obInitLogger(OB_LOG_USE_STD, OB_LOG_USE_KMSG);

  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  // ---------- Load config ----------

  ObContext* context = obCreateObContext(options.rootPrefix);
  ObConfig* config = &context->config;
  obLoadYamlConfig(config, options.configFile);

  // ---------- Mount persistent device ----------

  if (!obFindDevice(context)) {
    obLogE("Device %s not found", config->devicePath);
    return EXIT_FAILURE;
  }

  if (!obMountDevice(config->devicePath, context->devMountPoint)) {
    obLogE("Device mount (%s -> %s) failed", config->devicePath, context->devMountPoint);
    return EXIT_FAILURE;
  }

  \
  // ---------- Prepare /overlay ----------

  if (!obPrepareOverlay(context->overbootDir, config->tmpfsSize)) {
    obLogE("Cannot prepare overboot dir (%s)", context->overbootDir);
    return EXIT_FAILURE;
  }

  char upperPath[OB_DEV_PATH_MAX];
  sprintf(upperPath, "%s/upper", context->overbootDir);

  if (!config->useTmpfs) {
    char srcPath[OB_PATH_MAX];
    sprintf(srcPath, "%s/%s/upper", context->devMountPoint, config->repository);

    if (config->clearUpper) {
      obLogI("Clearing upper directory (%s)", srcPath);
      obRemoveDirR(srcPath);
      obMkpath(srcPath, OB_MKPATH_MODE);
    }
    else if (!obExists(srcPath)) {
      obMkpath(srcPath, OB_MKPATH_MODE);
    }

    obRbind(srcPath, upperPath);
  }


  // ---------- Move $rootmnt ----------

  char lowerPath[OB_PATH_MAX];

  sprintf(lowerPath, "%s/lower-root", context->overbootDir);
  obLogI("Moving %s to %s", context->root, lowerPath);
  obMove(context->root, lowerPath);

  // ---------- Mount overlayfs ----------

  char workPath[OB_DEV_PATH_MAX];
  sprintf(workPath, "%s/work", context->overbootDir);

  char repoPath[OB_PATH_MAX];
  sprintf(repoPath, "%s/%s", context->devMountPoint, config->repository);
  char layersPath[OB_PATH_MAX];
  sprintf(layersPath, "%s/%s", repoPath, OB_LAYERS_DIR_NAME);

//  char** layers = malloc(1 * sizeof(char*));

  uint8_t count = 0;
  ObLayerItem* topLayer = obCollectLayers(layersPath, config->headLayer, lowerPath, &count);

  if (count == 0 && !topLayer) {
    topLayer = calloc(1, sizeof(ObLayerItem));
    strcpy(topLayer->layerPath, lowerPath);
    topLayer->prev = NULL;
    count = 1;
  }

  char* layers[count];

  obLogI("Collected layers:");
  ObLayerItem* layerItem = topLayer;
  uint8_t i = 0;
  while (layerItem) {
    layers[count - i - 1] = layerItem->layerPath;
    obLogI("root path [%i]: %s", i, layerItem->layerPath);
    i += 1;
    layerItem = layerItem->prev;
  }

//  if (layerItem && layerItem->prev) {
//    layers[i] = layerItem->prev->layerPath;
//  }
//  else {
//    count -= 1;
//  }

  for (int j = 0; j < count; ++j) {
    obLogI("final layer %i: %s", j, layers[j]);
  }

  //  char** layers = NULL;
//  uint8_t layerCount = 0;
//  obCollectLayers(repoPath, context->headLayer,
//                  lowerPath, layers, &layerCount);

  if (!obMountOverlay(layers, count, upperPath, workPath, context->root)) {
    obLogE("Cannot mount overlay");
  }

//  for (uint8_t i = 0; i < layerCount; ++i) {
//    free(layers[i]);
//  }
//  free(layers);



  // ---------- Bind /overlay into $rootmnt ----------

  char bindedOverlay[OB_PATH_MAX];
  sprintf(bindedOverlay, "%s/%s", context->root, OB_USER_BINDINGS_DIR);
  obRbind(context->overbootDir, bindedOverlay);

  if (config->bindLayers) {
    char layersDir[OB_PATH_MAX];
    char bindedLayersDir[OB_PATH_MAX];
    sprintf(layersDir, "%s/layers", repoPath);
    sprintf(bindedLayersDir, "%s/layers", bindedOverlay);
    obMkpath(layersDir, OB_MKPATH_MODE);
    obMkpath(bindedLayersDir, OB_MKPATH_MODE);
    obRbind(layersDir, bindedLayersDir);
  }


  // ---------- Update fstab ----------

  char mtabPath[OB_PATH_MAX];
  sprintf(mtabPath, "%s/etc/mtab", config->prefix);

  updateFstab(context->root, mtabPath);


  // ---------- Bind durables ----------

  ObDurable* durable = config->durable;
  while (durable != NULL) {
    char persistentPath[OB_PATH_MAX];
    char bindPath[OB_PATH_MAX];

    sprintf(persistentPath, "%s/%s%s", repoPath, OB_DURABLES_DIR_NAME, durable->path);
    sprintf(bindPath, "%s%s", context->root, durable->path);

    if (!obExists(bindPath)) {
      obMkpath(bindPath, OB_MKPATH_MODE);
      obMkpath(persistentPath, OB_MKPATH_MODE);
    }
    else {
      bool isDir = obIsDirectory(bindPath);
      if (isDir && !obExists(persistentPath)) {
        obLogI("Persistent directory not found, creating: %s", persistentPath);
        obLogI("is dir: %i", obIsDirectory(persistentPath));
        obMkpath(persistentPath, OB_MKPATH_MODE);
        obLogI("is dir: %i", obIsDirectory(persistentPath));

        if (durable->copyOrigin) {
          obLogI("Copying origin from %s", bindPath);
          obSync(bindPath, persistentPath);
        }
      }
      else if (!isDir && !obExists(persistentPath)) {
        obLogI("This durable is not a directory");
        if (durable->copyOrigin) {
          obLogI("Copying original file from %s to %s", bindPath, persistentPath);
          if (!obCopyFile(bindPath, persistentPath)) {
            obLogE("Copying originl file failed");
          }
        }
        else {
          obCreateBlankFile(persistentPath);
        }
      }
    }

    obLogI("Binding durable: %s to %s", persistentPath, bindPath);
    obRbind(persistentPath, bindPath);
    durable = durable->next;
  }

  obFreeObContext(&context);
  return 0;
}
