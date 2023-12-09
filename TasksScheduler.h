#pragma once
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include "Tasks.h"
#include "Workers.h"
#include "Resources.h"

using namespace std;

// TODO store the tasks that are paused from rain

// TODO add workerIDs/worker assigned to a task

vector<int> workerIDS;

class TasksScheduler
{
private:
    vector<Task> tasks;
    vector<Task> lowPriorityTasks;
    vector<Task> mediumPriorityTasks;
    vector<Task> highPriorityTasks;

    // ratios for rotating between the queues for MSQ
    static const int highPriorityRatio = 5;
    static const int mediumPriorityRatio = 3;
    static const int lowPriorityRatio = 2;
    int totalRatio = highPriorityRatio + mediumPriorityRatio + lowPriorityRatio;

    pthread_mutex_t queueRotationMutex = PTHREAD_MUTEX_INITIALIZER;
    int currentQueueIndex = 0;  // high = 0, medium = 1, low = 2
    int queueRotationTimer = 0; // keeps track of how long its been since a queue switched

public:
    void scheduleTask(Task &task)
    {
        // high = 1, med = 2, low = 3
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
            Implemented through MLFQ (MultiLevel Feedback Queue)
            Returns a Task from one of the three arrays
            Returns "InvalidTask" Task if any task does not have feasibililty (edge case -- may never happen)
        */

        // calculating current time
        pthread_mutex_lock(&queueRotationMutex);
        int currentTime = queueRotationTimer;
        pthread_mutex_unlock(&queueRotationMutex);

        // prioritizing queue based on current rotation
        int queueIndex = calculateQueueIndex(currentTime);
        // index: high = 0, med = 1, low = 2
        vector<Task> &currentQueue = getQueueByIndex(queueIndex);

        // incrementing timer
        pthread_mutex_lock(&queueRotationMutex);
        queueRotationTimer++;
        pthread_mutex_unlock(&queueRotationMutex);

        return getTaskFromQueue(currentQueue, isRaining, availableMaterials, availableWorkers);
    }

    bool isTaskFeasible(Task &task, bool isRaining, const vector<vector<Resource>> &availableMaterials, vector<Worker> availableWorkers)
    {
        bool feasibilityFlag = true;
        int AWCounter = 0, brickCounter = 0, cementCounter = 0, toolCounter = 0, availBricks = 0, availCement = 0, availTools = 0;

        // Outdoor task become nonfeasible
        if (isRaining && !task.indoor)
        {
            return false;
        }

        // Counting Resources from Task
        for (size_t countRes = 0; countRes < task.resources.size(); countRes++)
        {
            if (task.resources[countRes].type == "Brick")
            {
                brickCounter++;
            }
            if (task.resources[countRes].type == "Cement")
            {
                cementCounter++;
            }
            if (task.resources[countRes].type == "Tool")
            {
                toolCounter++;
            }
        }
        // Counting resources from Availale Resources
        availBricks = availableMaterials[0].size();
        availCement = availableMaterials[1].size();
        availTools = availableMaterials[2].size();

        // Resources not Sufficient
        if (availBricks < brickCounter || availCement < cementCounter || availTools < toolCounter)
        {
            return false;
        }

        // Skilled work not available
        for (size_t skillLVL = 0; skillLVL < availableWorkers.size(); skillLVL++)
        {
            if (task.priority != availableWorkers[skillLVL].skillLevel)
            {
                for (size_t skillSET = 0; skillSET < availableWorkers[skillLVL].skillSet.size(); skillSET++)
                {
                    if (task.taskName == availableWorkers[skillLVL].skillSet[skillSET])
                    {
                        AWCounter++;

                        // storing worker IDs that have been assigned to Task
                        workerIDS.push_back(availableWorkers[skillLVL].workerId);
                    }
                }
            }
        }
        if (AWCounter < task.numWorkers)
        {
            return false;
        }

        return true;
    }

    int calculateQueueIndex(int currentTime)
    {
        int adjustedTime = currentTime % totalRatio;

        if (adjustedTime < highPriorityRatio)
        {
            return 0; // high prio queue
        }
        else if (adjustedTime < highPriorityRatio + mediumPriorityRatio)
        {
            return 1; // medium prio queue
        }
        else
        {
            return 2; // low prio queue
        }
    }
    vector<Task>& getQueueByIndex(int index)
    {
        switch (index)
        {
        case 0:
            return highPriorityTasks;
        case 1:
            return mediumPriorityTasks;
        case 2:
            return lowPriorityTasks;
        default:
            // will always set to High prio (edge case -- most likely wont happen)
            return highPriorityTasks;
        }
    }
    Task getTaskFromQueue(vector<Task> &priorityQueue, bool isRaining, vector<vector<Resource>> availableMaterials, vector<Worker> availableWorkers)
    {

        // now it rotates between the queues instead of high -> medium -> low
        for (auto iterate = priorityQueue.begin(); iterate != priorityQueue.end(); ++iterate)
        {
            if (isTaskFeasible(*iterate, isRaining, availableMaterials, availableWorkers))
            {
                // removes Task from the queue
                Task feasibleTask = *iterate;
                priorityQueue.erase(iterate);
                return feasibleTask;
            }
        }

        // if no feasible task is found return an "InvalidTask" (most likely case when -- 1. Rain 2. All outdoor tasks)
        cout << "No Current Feasible Task\n";
        return Task{"InvalidTask", 0, 0, {}, {}, 0, false};
    }
};