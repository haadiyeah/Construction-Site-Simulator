#pragma once
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include "Tasks.h"
#include "Workers.h"
#include "Resources.h"

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

            // priority 1 is high
            // priority 2 is medium
            // priority 3 is low

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

        Task getTask(bool isRaining, vector<vector<Resource>> availableMaterials, int availableWorkers){
            
            /* 
            
            This function returns a Task based on the priority, weather, human and materials.
            There are 3 lists of tasks for each priority.

            Indoor tasks returned if isRaining == true
            Task end of queue if Material low
            Task end of queue if Worker low

            */

           // need to change this to priority

           // need to add task return based on external variables
            Task selectTask;
            // getting tasks from arrays            
            if (!highPriorityTasks.empty()) {
                selectTask = highPriorityTasks.front();
                highPriorityTasks.erase(highPriorityTasks.begin());
            }
            else if (!mediumPriorityTasks.empty())
            {
                selectTask = mediumPriorityTasks.front();
                mediumPriorityTasks.erase(mediumPriorityTasks.begin());
            }
            else if (!lowPriorityTasks.empty())
            {
                selectTask = lowPriorityTasks.front();
                lowPriorityTasks.erase(lowPriorityTasks.begin());
            }

            return selectTask;

        }

};