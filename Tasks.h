#pragma once

#include "Resources.h"
#include <vector>
#include <string>   
using namespace std;

struct Task
{
    string taskName;
    int priority;               // 1 (High), 2 (Medium), 3 (Low)
    int time;                   // number to sleep
    vector<Resource> resources; // resources required for the task
    vector<int> assignedWorkers;        // ids of workers assigned to the task
    int numWorkers;             // number of workers required to implement this task
    bool indoor;               // true if task is indoor, false if outdoor
};

class TaskGenerator
{
    // • High Priority Tasks: Urgent repairs, foundation laying, critical structural work.
    // • Medium Priority Tasks: General construction tasks, bricklaying, cement mixing.
    // • Low Priority Tasks: Non-critical tasks, finishing touches, aesthetic elements.

private:
    // Creating 9 different types of task, 3 in each category
    Task lowPrioTasks[3] = {};
    Task medPrioTasks[3] = {};
    Task highPrioTasks[3] = {};
    string resourceTypes[3] = {"Brick", "Cement", "Tool"};

    void initTask(Task &t)
    {
        t.time = rand() % 10 + 1;
        int numResources = rand() % 5 + 1;
        for (int j = 0; j < numResources; j++)
        {
            Resource r;
            r.type = resourceTypes[rand() % 3];
            r.quality = 100;
            t.resources.push_back(r);
        }
        t.numWorkers = rand() % 5 + 1;
    }

public:
    TaskGenerator()
    {
        //u can change these task names idk anything about construction
        string lowPrioTaskNames[3] = {"Landscaping", "Decorating", "Painting"};
        bool lowPrioTaskIndoor[3] = {false, true, true};

        string medPrioTaskNames[3] = {"Scaffolding", "Bricklaying", "Cement Mixing"};
        bool medPrioTaskIndoor[3] = {false, true, false};

        //TODO add urgent repair
        string highPrioTaskNames[3] = {"Roofing", "Foundation Laying", "Structural Framing"};
        bool highPrioTaskIndoor[3] = {false, true, false};

        for (int i = 0; i < 3; i++)
        {
            highPrioTasks[i].taskName = highPrioTaskNames[i];
            highPrioTasks[i].priority = 1;
            highPrioTasks[i].indoor = highPrioTaskIndoor[i];
            initTask(highPrioTasks[i]);

            medPrioTasks[i].taskName = medPrioTaskNames[i];
            medPrioTasks[i].priority = 2;
            medPrioTasks[i].indoor = medPrioTaskIndoor[i];
            initTask(medPrioTasks[i]);

            lowPrioTasks[i].taskName = lowPrioTaskNames[i];
            lowPrioTasks[i].priority = 3;
            lowPrioTasks[i].indoor = lowPrioTaskIndoor[i];
            initTask(lowPrioTasks[i]);
        }
    }

    Task generateTask(int priority = 0)
    { // priority parameter is optional, if invalid passed it will randomize.
        if ((priority < 1 || priority > 3))
        {
            priority = rand() % 3 + 1;
        }

        switch (priority)
        {
        case 1:
            return highPrioTasks[rand() % 3];
        case 2:
            return medPrioTasks[rand() % 3];
        case 3:
            return lowPrioTasks[rand() % 3];
        default:
            break;
        }
        
        Task t;
        t.taskName = "Invalid task";
        return t; //will never reach here
    }
};