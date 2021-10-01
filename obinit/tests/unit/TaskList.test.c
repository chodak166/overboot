#include "unity.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>


typedef struct Task Task;
typedef struct Task* TaskPtr;
typedef void* TaskContext;
typedef bool (*TaskFunction)(TaskContext);

struct Task{
  TaskFunction exec;
  TaskFunction undo;
  TaskPtr next;
  TaskPtr previous;
  TaskContext context;
};

TaskPtr createTask(TaskFunction exec, TaskFunction undo, TaskContext context)
{
  TaskPtr task = malloc(sizeof(Task));
  task->exec = exec;
  task->undo = undo;
  task->previous = NULL;
  task->next = NULL;
  task->context = context;
  return task;
}

void freeTask(TaskPtr* task)
{
  free(*task);
  *task = NULL;
}

typedef struct {
  TaskPtr first;
  TaskPtr last;
} TaskList;

typedef TaskList* TaskListPtr;

TaskListPtr createTaskList()
{
  TaskList* list = malloc(sizeof(TaskList));
  list->first = list->last = NULL;
  return list;
}

void freeTaskList(TaskListPtr* taskList)
{
  while ((*taskList)->last != NULL) {
    TaskPtr previous = (*taskList)->last->previous;
    freeTask(&(*taskList)->last);
    (*taskList)->last = previous;
  }
  (*taskList)->first = NULL;

  free(*taskList);
  *taskList = NULL;
}

void appendTask(TaskListPtr taskList, TaskPtr task)
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

void addTask(TaskListPtr taskList, TaskFunction exec, TaskFunction undo, TaskContext context)
{
  TaskPtr task = createTask(exec, undo, context);
  appendTask(taskList, task);
}

bool callUndoChain(TaskPtr task)
{
  while (task != NULL) {
    if (task->undo != NULL) {
      task->undo(task->context); //TODO: catch return value
    }
    task = task->previous;
  }
  return true;
}

bool execTaskList(TaskListPtr taskList)
{
  TaskPtr task = taskList->first;
  while (task != NULL) {
    assert(task->exec != NULL);

    bool result = task->exec(task->context);
    if (!result) {
      callUndoChain(task);
      return false; //TODO: use enum
    }
    task = task->next;
  }
  return true;
}

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

bool successTask(TaskContext rawContext, char tag)
{
  TaskListTestContext* context = (TaskListTestContext*)rawContext;
  context->data[context->index] = tag;
  context->index += 1;
  return true;
}

bool successTaskA(TaskContext rawContext) {return successTask(rawContext, 'A');}
bool successTaskB(TaskContext rawContext) {return successTask(rawContext, 'B');}
bool successTaskC(TaskContext rawContext) {return successTask(rawContext, 'C');}

bool failTask(TaskContext rawContext)
{
  successTask(rawContext, 'F');
  return false;
}

bool undoTask(TaskContext rawContext)
{
  TaskListTestContext* context = (TaskListTestContext*)rawContext;
  context->index -= 1;
  context->data[context->index] = 'X';
  return true;
}

void test_createTaskList_shouldCreateNewTaskList()
{
  TaskListPtr taskList = createTaskList();
  TEST_ASSERT_NOT_NULL(taskList);

  freeTaskList(&taskList);
}

void test_freeTaskList_shouldFreeAndNullTaskList()
{
  TaskListPtr taskList = createTaskList();
  freeTaskList(&taskList);
  TEST_ASSERT_NULL(taskList);
}

void test_createTaskList_shouldCreateNullListEnds()
{
  TaskListPtr taskList = createTaskList();

  TEST_ASSERT_NULL(taskList->first);
  TEST_ASSERT_NULL(taskList->last);
  freeTaskList(&taskList);
}

void test_createTask_shouldCreateNewTaks()
{
  TaskPtr task = createTask(NULL, NULL, NULL);

  TEST_ASSERT_NOT_NULL(task);
  freeTask(&task);
}

void test_freeTask_shouldFreeAndNullTask()
{
  TaskPtr task = createTask(NULL, NULL, NULL);
  freeTask(&task);
  TEST_ASSERT_NULL(task);
}

void test_appendTask_shouldMakeTheTaskFirstAndLastOnEmptyList()
{
  TaskListPtr taskList = createTaskList();
  TaskPtr task = createTask(NULL, NULL, NULL);

  appendTask(taskList, task);

  TEST_ASSERT_EQUAL(task, taskList->first);
  TEST_ASSERT_EQUAL(task, taskList->last);

  freeTaskList(&taskList);
}

void test_appendTask_shouldUpdateLastTaskWhenListNotEmpty()
{
  TaskListPtr taskList = createTaskList();
  TaskPtr task1 = createTask(NULL, NULL, NULL);
  TaskPtr task2 = createTask(NULL, NULL, NULL);

  appendTask(taskList, task1);
  appendTask(taskList, task2);

  TEST_ASSERT_EQUAL(task1, taskList->first);
  TEST_ASSERT_EQUAL(task2, taskList->last);

  freeTaskList(&taskList);
}

void test_execTaskList_shouldExecuteAllTasksInOrder()
{
  TaskListPtr taskList = createTaskList();
  TaskListTestContext context = createTaskListTestContext();

  addTask(taskList, &successTaskA, NULL, &context);
  addTask(taskList, &successTaskB, NULL, &context);
  addTask(taskList, &successTaskC, NULL, &context);

  execTaskList(taskList);

  TEST_ASSERT_EQUAL('A', context.data[0]);
  TEST_ASSERT_EQUAL('B', context.data[1]);
  TEST_ASSERT_EQUAL('C', context.data[2]);

  freeTaskList(&taskList);
}

void test_execTaskList_shouldReturnTrueIfAllTasksSucceed()
{
  TaskListPtr taskList = createTaskList();
  TaskListTestContext context = createTaskListTestContext();
  addTask(taskList, &successTaskA, NULL, &context);

  bool result = execTaskList(taskList);

  TEST_ASSERT_TRUE(result);

  freeTaskList(&taskList);
}

void test_execTaskList_shouldReturnFalseIfAnyTasksFails()
{
  TaskListPtr taskList = createTaskList();
  TaskListTestContext context = createTaskListTestContext();

  addTask(taskList, &successTaskA, NULL, &context);
  addTask(taskList, &failTask, NULL, &context);
  addTask(taskList, &successTaskA, NULL, &context);

  bool result = execTaskList(taskList);

  TEST_ASSERT_FALSE(result);

  freeTaskList(&taskList);
}

void test_execTaskList_shouldExecAllUndoFunctionsFromFiledToFirst()
{
  TaskListPtr taskList = createTaskList();
  TaskListTestContext context = createTaskListTestContext();

  addTask(taskList, &successTaskA, &undoTask, &context);
  addTask(taskList, &successTaskB, &undoTask, &context);
  addTask(taskList, &failTask, &undoTask, &context);

  execTaskList(taskList);

  TEST_ASSERT_EQUAL('X', context.data[0]);
  TEST_ASSERT_EQUAL('X', context.data[1]);
  TEST_ASSERT_EQUAL('X', context.data[2]);

  freeTaskList(&taskList);
}
