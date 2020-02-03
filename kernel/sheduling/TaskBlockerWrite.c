#include "sheduling/TaskBlockerWrite.h"
#include "tasking.h"

bool blocker_write_can_unblock(TaskBlockerWrite *blocker, Task *task)
{
    __unused(task);

    return !fsnode_is_acquire(blocker->handle->node) && fsnode_can_write(blocker->handle->node, blocker->handle);
}

void blocker_write_unblock(TaskBlockerWrite *blocker, Task *task)
{
    fsnode_acquire_lock(blocker->handle->node, task->id);
}

TaskBlocker *blocker_write_create(FsHandle *handle)
{
    TaskBlockerWrite *write_blocker = __create(TaskBlockerWrite);

    write_blocker->blocker = (TaskBlocker){
        (TaskBlockerCanUnblock)blocker_write_can_unblock,
        (TaskBlockerUnblock)blocker_write_unblock,
    };

    write_blocker->handle = handle;

    return (TaskBlocker *)write_blocker;
}