// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObTaskList.h"

#include <stdlib.h>
#include <assert.h>

ObTaskPtr obCreateTask(ObTaskFunction exec, ObTaskFunction undo, ObTaskContext context)
{
  ObTaskPtr task = malloc(sizeof(ObTask));
  task->exec = exec;
  task->undo = undo;
  task->previous = NULL;
  task->next = NULL;
  task->context = context;
  return task;
}

void obFreeTask(ObTaskPtr* task)
{
  free(*task);
  *task = NULL;
}

ObTaskListPtr obCreateTaskList()
{
  ObTaskList* list = malloc(sizeof(ObTaskList));
  list->first = list->last = NULL;
  return list;
}

void obFreeTaskList(ObTaskListPtr* taskList)
{
  while ((*taskList)->last != NULL) {
    ObTaskPtr previous = (*taskList)->last->previous;
    obFreeTask(&(*taskList)->last);
    (*taskList)->last = previous;
  }
  (*taskList)->first = NULL;

  free(*taskList);
  *taskList = NULL;
}

void obAppendTask(ObTaskListPtr taskList, ObTaskPtr task)
{
  assert(taskList != NULL);

  task->previous = taskList->last;
  if (task->previous) {
    task->previous->next = task;
  }
  taskList->last = task;

  if (taskList->first == NULL) {
    taskList->first = task;
  }
}

void obAddTask(ObTaskListPtr taskList, ObTaskFunction exec, ObTaskFunction undo, ObTaskContext context)
{
  ObTaskPtr task = obCreateTask(exec, undo, context);
  obAppendTask(taskList, task);
}

bool obCallUndoChain(ObTaskPtr task)
{
  while (task != NULL) {
    if (task->undo != NULL) {
      task->undo(task->context); //TODO: catch return value
    }
    task = task->previous;
  }
  return true;
}

bool obExecTaskList(ObTaskListPtr taskList)
{
  ObTaskPtr task = taskList->first;
  while (task != NULL) {
    assert(task->exec != NULL);

    bool result = task->exec(task->context);
    if (!result) {
      obCallUndoChain(task);
      return false;
    }
    task = task->next;
  }
  return true;
}
