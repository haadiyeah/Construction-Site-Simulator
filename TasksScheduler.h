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
pthread_mutex_t highPriorityMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mediumPriorityMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lowPriorityMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t haltedTasksMutex = PTHREAD_MUTEX_INITIALIZER;
const char *rainingFifoPath = "/tmp/isRainingFifo";


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
            cout << "Task " << task.taskName << " added to high priority queue\n";
            highPriorityTasks.push_back(task);
            break;
        case 2:
            cout << "Task " << task.taskName << " added to medium priority queue\n";
            mediumPriorityTasks.push_back(task);
            break;
        case 3:
            cout << "Task " << task.taskName << " added to low priority queue\n";
            lowPriorityTasks.push_back(task);
            break;
        }
    }

    Task getTask(bool &isRaining, vector<vector<Resource>> &availableMaterials, vector<Worker> &availableWorkers, vector<Worker> &activeWorkers, vector<Worker> &backupWorkers)
    {
        cout << "Getting Task\n";
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

        Task task = getTaskFromQueue(currentQueue, queueIndex, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers);
        cout << "QUEUE Task: " << task.taskName << "\ttime: " << task.time << endl;
        return task;
    }

    bool isTaskFeasible(Task &task, bool &isRaining, const vector<vector<Resource>> &availableMaterials, vector<Worker> &availableWorkers, vector<Worker> &activeWorkers, vector<Worker> &backupWorkers)
    {

        bool feasibilityFlag = true;
        int AWCounter = 0, brickCounter = 0, cementCounter = 0, toolCounter = 0, availBricks = 0, availCement = 0, availTools = 0;

        // Outdoor task become nonfeasible
        if (isRaining && !task.indoor)
        {
            cout << "Task " << task.taskName << " is not feasible due to rain\n";
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
            cout << "Task " << task.taskName << " is not feasible due to insufficient resources\n";
            return false;
        }

        // Skilled work not available

        // SLindex - Skill Level index
        // SSindex - Skill Set index
        pthread_mutex_trylock(&idleWorkerMutex);
        vector<int> workerIDS;
        for (int i = 0; i < availableWorkers.size(); i++)
        {
            if (task.priority >= availableWorkers[i].skillLevel)
            {
                for (int j = 0; j < availableWorkers[i].skillSet.size(); j++)
                {
                    if (task.taskName == availableWorkers[i].skillSet[j])
                    {
                        AWCounter++;
                        // storing worker IDs that have been assigned to Task
                        workerIDS.push_back(availableWorkers[i].workerId);
                    }
                }
            }
        }

        pthread_mutex_unlock(&idleWorkerMutex);
        if (AWCounter < task.numWorkers)
        {
            cout << "Task " << task.taskName << " is not feasible due to insufficient skilled workers\n";
            return false;
        }

        // assinging idle workers to active workers
        pthread_mutex_trylock(&idleWorkerMutex);
        for (int i = 0; i < task.numWorkers; i++)
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

    int rainingFifoFd = open(rainingFifoPath, O_RDONLY | O_NONBLOCK);
    bool getRainingStatus()
    {
        bool isRaining;
        ssize_t bytesRead = read(rainingFifoFd, &isRaining, sizeof(isRaining));

        if (bytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No data was read from the FIFO, so continue with the next iteration of the loop
            }
            else
            {
                perror("ERROR! "); // Print the error message specified by errno
            }
        }
        //cout << "isRaining: " << isRaining << endl;
        return isRaining;
    }

    Task getTaskFromQueue(vector<Task> &priorityQueue, int queueIndex, bool &isRaining, vector<vector<Resource>> &availableMaterials, vector<Worker> &availableWorkers, vector<Worker> &activeWorkers, vector<Worker> &backupWorkers)
    {
        

        /*

        Now it rotates between the queues instead of high -> medium -> low
        Iteration in arrays will occur when it is raining
        Tasks will be pop -> push_back when not feasible and not raining

        Halted Tasks have highest priority. -- Use haltTask() to populate haltedTasks
        */

        if (!isRaining && !haltedTasks.empty()) // haltedTasks has most priority over other
        {
            while (!haltedTasks.empty())
            {
                Task haltedTask = haltedTasks.front();
                pthread_mutex_trylock(&haltedTasksMutex);
                haltedTasks.erase(haltedTasks.begin());
                pthread_mutex_unlock(&haltedTasksMutex);

                if (isTaskFeasible(haltedTask, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers))
                {
                    return haltedTask;
                }
                else
                {
                    cout << "Task " << haltedTask.taskName << " is not feasible\n";
                    pthread_mutex_trylock(&haltedTasksMutex);
                    haltedTasks.push_back(haltedTask);
                    pthread_mutex_unlock(&haltedTasksMutex);
                    sleep(5);
                }

            }
        }
        else
        {
            while (!(priorityQueue.empty()))
            {
                Task selectTask = priorityQueue.front();

                // lock the respective mutex 
                (queueIndex == 0) ? pthread_mutex_trylock(&highPriorityMutex) : (queueIndex == 1) ? pthread_mutex_trylock(&mediumPriorityMutex) : pthread_mutex_trylock(&lowPriorityMutex);
                priorityQueue.erase(priorityQueue.begin());
                (queueIndex == 0) ? pthread_mutex_unlock(&highPriorityMutex) : (queueIndex == 1) ? pthread_mutex_unlock(&mediumPriorityMutex) : pthread_mutex_unlock(&lowPriorityMutex);
                
                if (isTaskFeasible(selectTask, isRaining, availableMaterials, availableWorkers, activeWorkers, backupWorkers))
                {
                    cout << "Task " << selectTask.taskName << " selected from priority queue\n";
                    return selectTask;
                }
                else
                {
                    cout << "Task " << selectTask.taskName << " is not feasible\n";

                    (queueIndex == 0) ? pthread_mutex_trylock(&highPriorityMutex) : (queueIndex == 1) ? pthread_mutex_trylock(&mediumPriorityMutex) : pthread_mutex_trylock(&lowPriorityMutex);
                    priorityQueue.push_back(selectTask);
                    (queueIndex == 0) ? pthread_mutex_unlock(&highPriorityMutex) : (queueIndex == 1) ? pthread_mutex_unlock(&mediumPriorityMutex) : pthread_mutex_unlock(&lowPriorityMutex);
                    sleep(5);
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
        while (1)
        {
            switch (index)
            {
            case 0:
                if (!highPriorityTasks.empty())
                    return highPriorityTasks;
            case 1:
                if (!mediumPriorityTasks.empty())
                    return mediumPriorityTasks;
            case 2:
                if (!lowPriorityTasks.empty())
                    return lowPriorityTasks;
            default:
                index = 0; 
            }
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