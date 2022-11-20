// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ob/ObHash.h"
#include "ob/ObLogging.h"
#include "xxhash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SEED 0
#define BUFFER_SIZE 1024

XXH64_hash_t calculateXxHash64Stream(FILE* file)
{
    XXH64_state_t* const state = XXH64_createState();
    if (state == NULL) {
      return 0;
    }

    char buffer[BUFFER_SIZE];
    XXH64_hash_t const seed = HASH_SEED;
    if (XXH64_reset(state, seed) == XXH_ERROR) {
      XXH64_freeState(state);
      return 0;
    }

    while (!feof(file)) {
        size_t length = fread(buffer, sizeof(char), BUFFER_SIZE, file);
        if (XXH64_update(state, buffer, length) == XXH_ERROR) {
          XXH64_freeState(state);
          return 0;
        }
    }

    XXH64_hash_t const hash = XXH64_digest(state);
    XXH64_freeState(state);
    return hash;
}

// --------- public API ---------- //

uint64_t obCalcualateFileHash(const char* path)
{
  uint64_t result = 0;
  FILE* file = fopen(path, "rb");
  if (file) {
    result = calculateXxHash64Stream(file);
    fclose(file);
    if (result == 0) {
      obLogE("Cannot calculate hash value for file: %s", path);
    }
  }
  else {
    obLogE("Hash calculation error. Cannot open file: %s", path);
  }
  return result;
}

bool obWriteAsHexStr(uint64_t value, const char* outputPath)
{
  FILE* file = fopen(outputPath, "w");
  if (file) {
    // store it as 16-char hex
    fprintf(file, "%" PRIx64 "\n", value);
    fclose(file);
  }
  else {
    obLogE("Cannot open file for writing: %s", outputPath);
    return false;
  }
  return true;
}

uint64_t obReadHashValue(const char* txtFilePath)
{
  uint64_t result = 0;
  const size_t BUFSIZE = 17;
  char buffer[BUFSIZE];
  char* end = NULL;
  FILE* file = fopen(txtFilePath, "r");
  if (file) {

    if (fgets(buffer, BUFSIZE, file)) {
      obLogI("Read hash value: %s", buffer);
      result = strtoull(buffer, &end, 16);
    }
    else {
      obLogE("Cannot read hash value from: %s", txtFilePath);
    }
    fclose(file);
  }
  else {
    obLogE("Cannot open file for reading: %s", txtFilePath);
  }

  return result;
}

