// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObYamlParser.h"
#include "ob/ObLogging.h"

#include <yaml.h>


static char* pushKey(char* path, const yaml_char_t* key)
{
  strcat(path, ".");
  strcat(path, (const char*)key);
  return path;
}

static char* popKey(char* path)
{
  char* findResult = strrchr(path, '.');
  if (findResult != NULL) {
    *findResult = '\0';
  }
  return path;
}


bool obParseYamlFile(void* context, const char* path, ObYamlValueCallback valueCallback, ObYamlEntryCallback entryCallback)
{
  FILE *configFile = fopen(path, "r");
  yaml_parser_t parser;

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser)) {
    obLogE("Failed to initialize YAML parser");
    return false;
  }
  if(configFile == NULL) {
    obLogE("Failed to open file: %s", path);
    return false;
  }

  yaml_parser_set_input_file(&parser, configFile);

  yaml_token_t token;
  char itemPath[128] = "";
  bool isKey = false;
  do {
    yaml_parser_scan(&parser, &token);
    switch(token.type)
    {
    case YAML_KEY_TOKEN:
      isKey = true;
      break;
    case YAML_VALUE_TOKEN:
      isKey = false;
      break;
    case YAML_BLOCK_ENTRY_TOKEN:
      if (entryCallback) {
        entryCallback(context, itemPath);
      }
      pushKey(itemPath, (yaml_char_t*)"");
      break;
    case YAML_BLOCK_END_TOKEN:
      popKey(itemPath);
      break;
    case YAML_SCALAR_TOKEN:
      if (isKey) {
        pushKey(itemPath, token.data.scalar.value);
      }
      else {
        if (valueCallback) {
          valueCallback(context, itemPath, (char*)token.data.scalar.value);
        }
        popKey(itemPath);
      }
      isKey = false;
      break;
    default:;
    }
    if (token.type != YAML_STREAM_END_TOKEN) {
      yaml_token_delete(&token);
    }
  } while (token.type != YAML_STREAM_END_TOKEN);

  yaml_token_delete(&token);
  yaml_parser_delete(&parser);
  fclose(configFile);

  return true;
}
