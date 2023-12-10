#pragma once
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>

#include "Tasks.h"
#include "Workers.h"
#include "Resources.h"

using namespace std;

// TODO Add priority of Urgent Repairs Task

pthread_mutex_t queueRotationMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t idleWorkerMutex = PTHREAD_MUTEX_INITIALIZER;

class TasksScheduler
{
private:
    vector<Task> tasks;
    vector<Task> lowPriorityTasks;
    vector<Task> mediumPriorityTasks;
    vector<Task> highPriorityTasks;
    vector<Task> haltedTasks;

    // ratios for rotating between the queues for MSQ
    static const int highPriorityRatio = 5;
    static const int mediumPriorityRatio = 3;
    static const int lowPriorityRatio = 2;
    int totalRatio = highPriorityRatio + mediumPriorityRatio + lowPriorityRatio;

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

    Task getTask(bool isRaining, vector<vector<Resource>> availableMaterials, vector<Worker> availableWorkers, vector<Worker> activeWorkers, vector<Worker> backupWorkers)
    {

        /*
            Implemented through MLFQ (MultiLevel Feedback Queue)
            Returns a Task from one of the three arrays
            Returns a Task from halted Tasks if any exist
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

        return getTaskFromQueue(currentQueue, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers);
    }

    bool isTaskFeasible(Task &task, bool isRaining, const vector<vector<Resource>> &availableMaterials, vector<Worker> availableWorkers, vector<Worker> activeWorkers, vector<Worker> backupWorkers)
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

        // SLindex - Skill Level index
        // SSindex - Skill Set index
        pthread_mutex_trylock(&idleWorkerMutex);
        vector<int> workerIDS;
        for (int SLindex = 0; SLindex < availableWorkers.size(); SLindex++)
        {
            if (task.priority != availableWorkers[SLindex].skillLevel)
            {
                for (int SSindex = 0; SSindex < availableWorkers[SLindex].skillSet.size(); SSindex++)
                {
                    if (task.taskName == availableWorkers[SLindex].skillSet[SSindex])
                    {
                        AWCounter++;

                        // storing worker IDs that have been assigned to Task
                        workerIDS.push_back(availableWorkers[SLindex].workerId);
                    }
                }
            }
        }

        pthread_mutex_unlock(&idleWorkerMutex);
        if (AWCounter < task.numWorkers)
        {
            return false;
        }

        // assinging idle workers to active workers
        pthread_mutex_trylock(&idleWorkerMutex);
        for (int i = 0; i < workerIDS.size(); i++)
        {
            task.assignedWorkers.push_back(workerIDS[i]);
            for (auto iterator = availableWorkers.begin(); iterator != availableWorkers.end(); iterator++)
            {
                if (iterator->workerId == workerIDS[i])
                {
                    Worker assignWorker = *iterator;

                    // erase assign worker from idle worker
                    iterator = availableWorkers.erase(iterator);
                    activeWorkers.push_back(assignWorker);

                    // removing worker from backup to idle
                    if (!(backupWorkers.empty()))
                    {
                        Worker newWorker = backupWorkers.front();
                        backupWorkers.erase(backupWorkers.begin());
                        availableWorkers.push_back(newWorker);
                    }
                    else
                    {
                        cout << "backup not available\n";
                    }
                    break;
                }
            }
        }
        pthread_mutex_unlock(&idleWorkerMutex);

        return true;
    }
    Task getTaskFromQueue(vector<Task> &priorityQueue, bool isRaining, vector<vector<Resource>> availableMaterials, vector<Worker> availableWorkers, vector<Worker> activeWorkers, vector<Worker> backupWorkers)
    {

        /*

        Now it rotates between the queues instead of high -> medium -> low
        Iteration in arrays will occur when it is raining
        Tasks will be pop -> push_back when not feasible and not raining

        Halted Tasks have highest priority. -- Use haltTask() to populate haltedTasks
        */


        if (!isRaining && !haltedTasks.empty()) // haltedTasks has most priority over other
        {
            Task haltedTask = haltedTasks.front();
            haltedTasks.erase(haltedTasks.begin());

            if (isTaskFeasible(haltedTask, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers))
            {
                return haltedTask;
            }
            else
            {
                haltedTasks.push_back(haltedTask);
            }
        }
        else
        {
            if (!(isRaining))
            {
                if (!(priorityQueue.empty()))
                {
                    Task selectTask = priorityQueue.front();
                    priorityQueue.erase(priorityQueue.begin());

                    if (isTaskFeasible(selectTask, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers))
                    {
                        return selectTask;
                    }
                    else
                    {
                        priorityQueue.push_back(selectTask);
                    }
                }
            }
            else
            {
                for (auto iterate = priorityQueue.begin(); iterate != priorityQueue.end(); ++iterate)
                {
                    if (isTaskFeasible(*iterate, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers))
                    {
                        // removes Task from the queue and returns feasible
                        Task feasibleTask = *iterate;
                        priorityQueue.erase(iterate);
                        return feasibleTask;
                    }
                }
            }
        }

        // DEPRECIATED
        // if no feasible task is found return an "InvalidTask" (most likely case when -- 1. Rain 2. All outdoor tasks)
        // cout << "No Current Feasible Task\n";
        // return Task{"InvalidTask", 0, 0, {}, {}, 0, false};
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
    vector<Task> &getQueueByIndex(int index)
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

    void haltTask(Task &task)
    {
        haltedTasks.push_back(task);
    }

    // DEPRECIATED
    // helper function for getting task by Name
    /*
    Task findTaskByName(string taskName)
    {
        for (const auto &task : tasks)
        {
            if (task.taskName == taskName)
            {
                return task;
            }
        }
        return Task{};
    }
    */
};