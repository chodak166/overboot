// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBTASKLIST_H
#define OBTASKLIST_H

#include <stdbool.h>

typedef struct ObTask ObTask;
typedef struct ObTask* ObTaskPtr;
typedef void* ObTaskContext;
typedef bool (*ObTaskFunction)(ObTaskContext);

struct ObTask{
  ObTaskFunction exec;
  ObTaskFunction undo;
  ObTaskPtr next;
  ObTaskPtr previous;
  ObTaskContext context;
};

typedef struct {
  ObTaskPtr first;
  ObTaskPtr last;
} ObTaskList;

typedef ObTaskList* ObTaskListPtr;

ObTaskPtr obCreateTask(ObTaskFunction exec,
                       ObTaskFunction undo,
                       ObTaskContext context);

void obFreeTask(ObTaskPtr* task);

ObTaskListPtr obCreateTaskList();

void obFreeTaskList(ObTaskListPtr* taskList);

void obAppendTask(ObTaskListPtr taskList, ObTaskPtr task);

void obAddTask(ObTaskListPtr taskList, ObTaskFunction exec,
               ObTaskFunction undo, ObTaskContext context);

bool obCallUndoChain(ObTaskPtr task);

bool obExecTaskList(ObTaskListPtr taskList);

#endif // OBTASKLIST_H
