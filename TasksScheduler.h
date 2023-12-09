#pragma once
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include "Tasks.h"

using namespace std;

class TasksScheduler
{
    private:
        vector<Task> tasks;
        vector<Task> lowPriorityTasks;
        vector<Task> mediumPriorityTasks;
        vector<Task> highPriorityTasks;

    public:
        void scheduleTask(Task& task){
            tasks.push_back(task);
            switch (task.priority)
            {
            case 1:
                highPriorityTasks.push_back(task);
                break;
            case 2:
                mediumPriorityTasks.push_back(task);
                break;
            case 3:
                lowPriorityTasks.push_back(task);
                break;
            }
        }

        Task getTask(){
            
            
        }
};