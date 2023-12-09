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
    void scheduleTask(Task &task)
    {

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

    Task getTask(bool isRaining, vector<vector<Resource>> availableMaterials, vector<Worker> availableWorkers)
    {

        /*

        This function returns a Task based on the priority, weather, human and materials.
        There are 3 lists of tasks for each priority.

        Indoor tasks returned if isRaining == true
        */

        // need to change this to multilevel feedback queue

        // need to add task return based on external variables

        for (auto iterate = highPriorityTasks.begin(); iterate != highPriorityTasks.end(); ++iterate)
        {
            if (isTaskFeasible(*iterate, isRaining, availableMaterials, availableWorkers))
            {
                Task feasibleTask = *iterate;
                highPriorityTasks.erase(iterate);
                return feasibleTask;
            }
        }

        for (auto iterate = mediumPriorityTasks.begin(); iterate != mediumPriorityTasks.end(); ++iterate)
        {
            if (isTaskFeasible(*iterate, isRaining, availableMaterials, availableWorkers))
            {
                Task feasibleTask = *iterate;
                mediumPriorityTasks.erase(iterate);
                return feasibleTask;
            }
        }

        for (auto iterate = lowPriorityTasks.begin(); iterate != lowPriorityTasks.end(); ++iterate)
        {
            if (isTaskFeasible(*iterate, isRaining, availableMaterials, availableWorkers))
            {
                Task feasibleTask = *iterate;
                lowPriorityTasks.erase(iterate);
                return feasibleTask;
            }
        }
    }

    bool isTaskFeasible(Task &task, bool isRaining, const vector<vector<Resource>> &availableMaterials, vector<Worker> availableWorkers)
    {
        if (isRaining && !task.indoor)
        {
            // Outdoor task become nonfeasible
            return false;
        }

        return true;
    }
};