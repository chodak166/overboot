#include "unity.h"

#include "ObTaskList.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
  int index;
  char data[3];
} TaskListTestContext;

TaskListTestContext createTaskListTestContext()
{
  TaskListTestContext context;
  context.index = 0;
  context.data[0] = context.data[1] = context.data[2] = 0;
  return context;
}

bool successTask(ObTaskContext rawContext, char tag)
{
  TaskListTestContext* context = (TaskListTestContext*)rawContext;
  context->data[context->index] = tag;
  context->index += 1;
  return true;
}

bool successTaskA(ObTaskContext rawContext) {return successTask(rawContext, 'A');}
bool successTaskB(ObTaskContext rawContext) {return successTask(rawContext, 'B');}
bool successTaskC(ObTaskContext rawContext) {return successTask(rawContext, 'C');}

bool failTask(ObTaskContext rawContext)
{
  successTask(rawContext, 'F');
  return false;
}

bool undoTask(ObTaskContext rawContext)
{
  TaskListTestContext* context = (TaskListTestContext*)rawContext;
  context->index -= 1;
  context->data[context->index] = 'X';
  return true;
}

void test_createTaskList_shouldCreateNewTaskList()
{
  ObTaskListPtr taskList = obCreateTaskList();
  TEST_ASSERT_NOT_NULL(taskList);

  obFreeTaskList(&taskList);
}

void test_freeTaskList_shouldFreeAndNullTaskList()
{
  ObTaskListPtr taskList = obCreateTaskList();
  obFreeTaskList(&taskList);
  TEST_ASSERT_NULL(taskList);
}

void test_createTaskList_shouldCreateNullListEnds()
{
  ObTaskListPtr taskList = obCreateTaskList();

  TEST_ASSERT_NULL(taskList->first);
  TEST_ASSERT_NULL(taskList->last);
  obFreeTaskList(&taskList);
}

void test_createTask_shouldCreateNewTaks()
{
  ObTaskPtr task = obCreateTask(NULL, NULL, NULL);

  TEST_ASSERT_NOT_NULL(task);
  obFreeTask(&task);
}

void test_freeTask_shouldFreeAndNullTask()
{
  ObTaskPtr task = obCreateTask(NULL, NULL, NULL);
  obFreeTask(&task);
  TEST_ASSERT_NULL(task);
}

void test_appendTask_shouldMakeTheTaskFirstAndLastOnEmptyList()
{
  ObTaskListPtr taskList = obCreateTaskList();
  ObTaskPtr task = obCreateTask(NULL, NULL, NULL);

  obAppendTask(taskList, task);

  TEST_ASSERT_EQUAL(task, taskList->first);
  TEST_ASSERT_EQUAL(task, taskList->last);

  obFreeTaskList(&taskList);
}

void test_appendTask_shouldUpdateLastTaskWhenListNotEmpty()
{
  ObTaskListPtr taskList = obCreateTaskList();
  ObTaskPtr task1 = obCreateTask(NULL, NULL, NULL);
  ObTaskPtr task2 = obCreateTask(NULL, NULL, NULL);

  obAppendTask(taskList, task1);
  obAppendTask(taskList, task2);

  TEST_ASSERT_EQUAL(task1, taskList->first);
  TEST_ASSERT_EQUAL(task2, taskList->last);

  obFreeTaskList(&taskList);
}

void test_execTaskList_shouldExecuteAllTasksInOrder()
{
  ObTaskListPtr taskList = obCreateTaskList();
  TaskListTestContext context = createTaskListTestContext();

  obAddTask(taskList, &successTaskA, NULL, &context);
  obAddTask(taskList, &successTaskB, NULL, &context);
  obAddTask(taskList, &successTaskC, NULL, &context);

  obExecTaskList(taskList);

  TEST_ASSERT_EQUAL('A', context.data[0]);
  TEST_ASSERT_EQUAL('B', context.data[1]);
  TEST_ASSERT_EQUAL('C', context.data[2]);

  obFreeTaskList(&taskList);
}

void test_execTaskList_shouldReturnTrueIfAllTasksSucceed()
{
  ObTaskListPtr taskList = obCreateTaskList();
  TaskListTestContext context = createTaskListTestContext();
  obAddTask(taskList, &successTaskA, NULL, &context);

  bool result = obExecTaskList(taskList);

  TEST_ASSERT_TRUE(result);

  obFreeTaskList(&taskList);
}

void test_execTaskList_shouldReturnFalseIfAnyTasksFails()
{
  ObTaskListPtr taskList = obCreateTaskList();
  TaskListTestContext context = createTaskListTestContext();

  obAddTask(taskList, &successTaskA, NULL, &context);
  obAddTask(taskList, &failTask, NULL, &context);
  obAddTask(taskList, &successTaskA, NULL, &context);

  bool result = obExecTaskList(taskList);

  TEST_ASSERT_FALSE(result);

  obFreeTaskList(&taskList);
}

void test_execTaskList_shouldExecAllUndoFunctionsFromFiledToFirst()
{
  ObTaskListPtr taskList = obCreateTaskList();
  TaskListTestContext context = createTaskListTestContext();

  obAddTask(taskList, &successTaskA, &undoTask, &context);
  obAddTask(taskList, &successTaskB, &undoTask, &context);
  obAddTask(taskList, &failTask, &undoTask, &context);

  obExecTaskList(taskList);

  TEST_ASSERT_EQUAL('X', context.data[0]);
  TEST_ASSERT_EQUAL('X', context.data[1]);
  TEST_ASSERT_EQUAL('X', context.data[2]);

  obFreeTaskList(&taskList);
}
