// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObInitTasks.h"

#include "ObTaskList.h"
#include "ObDeinit.h"
#include "ob/ObInit.h"
#include "ob/ObLogging.h"
#include "ob/ObJobs.h"
#include <stdlib.h>

static bool checkRollback(ObContext* context)
{
  return !context->config.rollback;
}

static ObTaskListPtr createObInitTaskList(ObContext* context)
{
  ObTaskPtr task = NULL;
  ObTaskListPtr tasks = obCreateTaskList();

  task = obCreateTask((ObTaskFunction)obInitPersistentDevice,
                      (ObTaskFunction)obDeinitPersistentDevice,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitLock,
                      NULL,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obExecPreInitJobs,
                      NULL,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitOverbootDir,
                      (ObTaskFunction)obDeinitOverbootDir,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitLowerRoot,
                      (ObTaskFunction)obDeinitLowerRoot,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitOverlayfs,
                      (ObTaskFunction)obDeinitOverlayfs,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitManagementBindings,
                      (ObTaskFunction)obDeinitManagementBindings,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitFstab,
                      (ObTaskFunction)obDeinitFstab,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obInitDurables,
                      (ObTaskFunction)obDeinitDurables,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)checkRollback,
                      NULL,
                      context);
  obAppendTask(tasks, task);

  task = obCreateTask((ObTaskFunction)obUnsetLock,
                      NULL,
                      context);

  obAppendTask(tasks, task);

  return tasks;
}

// --------- public API ---------- //

bool obExecObInitTasks(ObContext* context)
{
  ObTaskListPtr tasks = createObInitTaskList(context);
  obLogI("Executing obinit tasks");
  bool result = obExecTaskList(tasks);

  if (result && obErrorOccurred()) {
    obLogE("An error occurred during initialization, please see full log for more details");
    result = false;
    obCallUndoChain(tasks->last);
  }

  obFreeTaskList(&tasks);
  return result;
}
