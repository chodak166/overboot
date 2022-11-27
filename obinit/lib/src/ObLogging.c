// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ob/ObLogging.h"

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#define OB_LOG_MAX 4096

#define IGNORE_RETURN (void)!

static struct {
  bool stdOut;
  bool kmsgOut;
} obLoggerSettings = {true, false};


static bool _obErrorOccurred = false;

static void obWriteStdLog(const char* log)
{
  fprintf(stdout, "%s", log);
  fflush(stdout);
}

static void obWriteStdLogE(const char* log)
{
  fprintf(stderr, "%s", log);
  fflush(stderr);
}


static void obWriteKmsgLog(const char* log)
{
  int fd = open("/dev/kmsg", O_RDWR);
  if( fd<0 ) {
    obWriteStdLogE("problem opening /dev/kmsg\n");
    return;
  }
  IGNORE_RETURN write(fd, log, strlen(log)+1);
  close(fd);
}

static void obWriteLog(const char* log)
{
  if (obLoggerSettings.stdOut) {
    obWriteStdLog(log);
  }
  if (obLoggerSettings.kmsgOut) {
    obWriteKmsgLog(log);
  }
}

static void obWriteLogE(const char* log)
{
  if (obLoggerSettings.stdOut) {
    obWriteStdLogE(log);
  }
  if (obLoggerSettings.kmsgOut) {
    obWriteKmsgLog(log);
  }
}


static char* obDecorateLog(char* log, const char* msg, const char* severity, va_list args)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  char formattedMsg[OB_LOG_MAX] = "";

  vsprintf(formattedMsg, msg, args);

  sprintf(log, "%d-%02d-%02d %02d:%02d:%02d OBINIT %s: %s\n",
          tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
          severity, formattedMsg);

  return log;
}


// --------- public API ---------- //


void obInitLogger(bool stdOut, bool kmsgOut)
{
  obLoggerSettings.stdOut = stdOut;
  obLoggerSettings.kmsgOut = kmsgOut;
}

void obLogI(const char* msg, ...)
{
  char log[OB_LOG_MAX] = "";
  va_list args;
  va_start(args, msg);
  obDecorateLog(log, msg, "INFO", args);
  va_end(args);
  obWriteLog(log);
}

void obLogW(const char* msg, ...)
{
  char log[OB_LOG_MAX] = "";
  va_list args;
  va_start(args, msg);
  obDecorateLog(log, msg, "WARNING", args);
  va_end(args);
  obWriteLog(log);
}

void obLogE(const char* msg, ...)
{
  char log[OB_LOG_MAX] = "";
  va_list args;
  va_start(args, msg);
  obDecorateLog(log, msg, "ERROR", args);
  va_end(args);
  obWriteLogE(log);

  if (!_obErrorOccurred) {
    _obErrorOccurred = true;
  }
}

bool obErrorOccurred()
{
  return _obErrorOccurred;
}

void clearErrorOccurrence()
{
  _obErrorOccurred = false;
}
