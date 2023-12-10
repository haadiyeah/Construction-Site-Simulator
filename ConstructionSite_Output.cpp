#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <algorithm>
#include <semaphore.h>
#include <semaphore.h>

#include "Resources.h"
#include "Tasks.h"
#include "Workers.h"
#include "TasksScheduler.h"

using namespace std;

bool isRunning = true;
bool isRaining = false;

int parentToChild[2];
int childToParent[2];
const char *runningFifoPath = "/tmp/isRunningFifo";
const char *rainingFifoPath = "/tmp/isRainingFifo";
const char *idleWorkersFifoPath = "/tmp/idleWorkersFifo";
const char *workingWorkersFifoPath = "/tmp/workingWorkersFifo";
const char *workerLeaveAlert = "/tmp/workerLeaveAlert";
const char *workerDeathAlert = "/tmp/workerDeathAlert";
const char *workerPromotionAlert = "/tmp/workerPromotionAlert";

const int MAX_CAPACITY = 50; // max capacity for each type of resource
TaskGenerator taskGenerator;
TasksScheduler tasksScheduler;
WorkerGenerator workerGenerator;

//--- Material related variables --- 
vector<vector<Resource>> materials(3); // 0 - bricks, 1 - cement, 2 - tools
pthread_mutex_t materialsMutex[3] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

// --- Worker related variables ---
pthread_mutex_t backupWorkerMutex = PTHREAD_MUTEX_INITIALIZER;
vector<Worker> backupWorkers;          // virtual workers
vector<Worker> idleWorkers;
vector<Worker> workingWorkers;

void *supplyFactory(void *arg)
{
    int demands[3] = {0, 0, 0}; //0 = brick, 1 = cement, 2 = tool

    while (isRunning)
    {
        for (int i = 0; i < 3; i++)
        {
            //If the resource is eligible for production && the demand is the highest (most consumed resource)
            if (materials[i].size() < MAX_CAPACITY )
            {
                if(demands[i] != max({demands[0], demands[1], demands[2]})) {
                    if(rand()%2) { //50% less chance of producing the resource if it is not the most demanded resource
                        continue;
                    }
                }
                sem_wait(&empty[i]); // Wait for an empty slot
                pthread_mutex_lock(&materialsMutex[i]);
                Resource r;
                switch (i)
                {
                case 0:
                    r.type = "Brick";
                    break;
                case 1:
                    r.type = "Cement";
                    break;
                case 2:
                    r.type = "Tool";
                    break;
                }
                r.quality = 100;
                cout << "Supply factory: Produced 1 " << r.type << endl;
                materials[i].push_back(r);
                demands[i]++;
                pthread_mutex_unlock(&materialsMutex[i]);
                sem_post(&full[i]); // Signal that a resource is available
            }
        }
        sleep(1); // supply after every 5 seconds
    }
    pthread_exit(NULL);
}

void *materialDegredation(void *arg)
{ // degrades the first pushed resource of each type
    // cout << "Material Degredation: Degredation started" << endl;
    while (isRunning)
    {
        for (int i = 0; i < 3; i++)
        {
            if (!materials[i].empty())
            {
                pthread_mutex_lock(&materialsMutex[i]);
                Resource r = materials[i].front();
                // materials[i].pop_back(); // remove the first pushed resource
                r.quality -= 10;
                // cout << "Material Degredation: " << r.type << " quality is " << r.quality << endl;
                if (r.quality > 0)
                {
                    // materials[i].push_back(r);
                }
                else
                {
                    switch (i)
                    {
                    case 0:
                        cout << "Material Degredation: Brick quality is 0 and brick is destroyed" << endl;
                        // pop
                        break;
                    case 1:
                        cout << "Material Degredation: Cement quality is 0 and cement is destroyed" << endl;
                        break;
                    case 2:
                        cout << "Material Degredation: Tool quality is 0 and tool is destroyed" << endl;
                        break;
                    }
                }
                pthread_mutex_unlock(&materialsMutex[i]);
            }
        }
        sleep(8); // degredation after every 8 seconds
    }
    pthread_exit(NULL);
}

void* workerFatigue(void* arg) {
    while(isRunning) {
        for(int i = 0; i < backupWorkers.size(); i++) {
            pthread_mutex_trylock(&backupWorkerMutex);
            backupWorkers[i].fatigue -= 10;
            if(backupWorkers[i].fatigue < 0) {
                backupWorkers[i].fatigue = 0;
            }
        }
        sleep(10); // fatigue decreases after every 10 seconds
    }
    pthread_exit(NULL);
}

void *taskCreation(void *arg)
{
    while (isRunning)
    {
        Task task = taskGenerator.generateTask();
        cout << "Task Creation: Task " << task.taskName << " created" << endl;
        tasksScheduler.scheduleTask(task);

        sleep(10); // create task after every 10 seconds
    }
    pthread_exit(NULL);
}

Worker getWorkingWorker(int id)
{
    for (int i = 0; i < workingWorkers.size(); i++)
    {
        if (workingWorkers[i].workerId == id)
        {
            Worker worker = workingWorkers[i];
            workingWorkers.erase(workingWorkers.begin() + i);
            return worker;
        }
    }    
}

void *execution(void *arg)
{
    cout << "in thread Execution: Execution started" << endl;

    Task task = *(Task *)arg;
    cout << "Task time: " << task.time << endl;

    bool rain;

    while (read(parentToChild[0], &rain, sizeof(rain)))
    {
        cout << "in thread Execution: Waiting for rain status" << endl;
        cout << "Rain read: " << rain << endl;

        if (!task.indoor && rain)
        {
            int time = task.time * -1;
            write(childToParent[1], &time, sizeof(time));
            cout << "Task " << task.taskName << " interrupted!" << endl;
            break;
        }

        task.time--;
        cout << "Task " << task.taskName << " is being executed. Time remaining: " << task.time << endl;
        write(childToParent[1], &task.time, sizeof(task.time));
        sleep(1);

        if (task.time == 0)
        {
            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}

// function to update worker lists in pipe in order to be read by the other process
void updateWorkersLists()
{
    // close the fifos to ensure overwriting old data instead of appending
    close(open(idleWorkersFifoPath, O_WRONLY | O_NONBLOCK));
    close(open(workingWorkersFifoPath, O_WRONLY | O_NONBLOCK));

    int idleWorkersFifoFd = open(idleWorkersFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
    if (idleWorkersFifoFd == -1)
    {
        cerr << "Failed to open " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
        // return;
    }

    int workingWorkersFifoFd = open(workingWorkersFifoPath, O_WRONLY | O_NONBLOCK);
    if (workingWorkersFifoFd == -1)
    {
        cerr << "Failed to open " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
        // return;
    }

    vector<string> serializedIdleWorkers = serializeWorkers(idleWorkers);
    vector<string> serializedWorkingWorkers = serializeWorkers(workingWorkers);

    ostringstream ossIdle;
    ostringstream ossWorking;
    for (const string &serializedWorker : serializedIdleWorkers)
    {
        ossIdle << serializedWorker << "\n";
    }
    for (const string &serializedWorker : serializedWorkingWorkers)
    {
        ossWorking << serializedWorker << "\n";
    }

    string idleOutput = ossIdle.str();
    string workingOutput = ossWorking.str();

    ssize_t written = write(idleWorkersFifoFd, idleOutput.c_str(), idleOutput.length());
    if (written == -1)
    {
        cerr << "Failed to write to " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
    }
    else if (written < idleOutput.length())
    {
        cerr << "Buffer too small when writing to " << idleWorkersFifoPath << endl;
    }

    ssize_t written2 = write(workingWorkersFifoFd, workingOutput.c_str(), workingOutput.length());
    if (written2 == -1)
    {
        cerr << "Failed to write to " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
    }
    else if (written2 < workingOutput.length())
    {
        cerr << "Buffer too small when writing to " << workingWorkersFifoPath << endl;
    }
}

void *tasksExecution(void *arg) //
{
    cout << "Tasks Execution: Tasks execution started" << endl;

    while (isRunning)
    {
        cout << "-" << endl;
        pipe(parentToChild);
        pipe(childToParent);
        cout << "Tasks Execution: Pipes created" << endl;

        Task task = tasksScheduler.getTask(isRaining, materials, idleWorkers, workingWorkers, backupWorkers);
        updateWorkersLists();
        // cout << "Task got" << endl;
        // Task task = taskGenerator.generateTask();
        int initialTime = task.time;
        cout << "Tasks Execution: Task " << task.taskName << " is being executed" << endl;
        cout << "Tasks Execution: Task time: " << task.time << endl;

        pid_t pid = fork();

        if (pid == 0)
        {
            close(parentToChild[1]);
            close(childToParent[0]);

            pthread_t thread;
            pthread_create(&thread, NULL, execution, &task);

            pthread_join(thread, NULL);

            exit(0);
        }
        else if (pid > 0)
        {
            close(parentToChild[0]);
            close(childToParent[1]);

            write(parentToChild[1], &isRaining, sizeof(isRaining));
            int time;
            while (read(childToParent[0], &time, sizeof(time)) && time > 0)
            {
                write(parentToChild[1], &isRaining, sizeof(isRaining));
            }

            wait(NULL);

            if (time < 0)
            {
                time *= -1;
                task.time = time;
                cout << "Task " << task.taskName << " interrupted! Time remaining: " << task.time << endl;


                for (int i = 0; i < task.assignedWorkers.size(); i++)
                {
                    pthread_mutex_trylock(&backupWorkerMutex);
                    Worker worker = getWorkingWorker(task.assignedWorkers[i]);
                    worker.fatigue += (initialTime - task.time) * 5;    // fatigue increases by 5 for every second of work
                    backupWorkers.push_back(worker);
                    pthread_mutex_unlock(&backupWorkerMutex);

                    cout << "Worker " << worker.workerId << " is now going to backup, with fatigue: " << worker.fatigue << endl;
                }

                tasksScheduler.haltTask(task);  // change to aadd to halted tasks
            }
            else
            {
                task.time = time;

                cout << "Task " << task.taskName << " completed!" << endl;
                // release workers

                cout << "Num Workers released: " << task.assignedWorkers.size() << endl;
                for (int i = 0; i < task.assignedWorkers.size(); i++)
                {
                    pthread_mutex_trylock(&backupWorkerMutex);
                    Worker worker = getWorkingWorker(task.assignedWorkers[i]);
                    worker.fatigue += (initialTime - task.time) * 5;    // fatigue increases by 5 for every second of work
                    backupWorkers.push_back(worker);
                    pthread_mutex_unlock(&backupWorkerMutex);

                    cout << "Worker " << worker.workerId << " is now going to backup, with fatigue: " << worker.fatigue << endl;
                }
                updateWorkersLists();
            }
        }
        else
        {
            cout << "Error occurred in tasks execution " << endl;
        }

        close(parentToChild[0]);
        close(parentToChild[1]);
        close(childToParent[0]);
        close(childToParent[1]);
    }
    pthread_exit(NULL);
}

void *checkRunning(void *arg)
{
    int runningFifoFd = open(runningFifoPath, O_RDONLY | O_NONBLOCK); // Open pipe for reading in non-blocking mode
    int rainingFifoFd = open(rainingFifoPath, O_RDONLY | O_NONBLOCK);

    while (isRunning)
    {
        ssize_t bytesRead = read(runningFifoFd, &isRunning, sizeof(isRunning));
        ssize_t bytesRead2 = read(rainingFifoFd, &isRaining, sizeof(isRaining));

        if (bytesRead == -1 || bytesRead2 == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No data was read from the FIFO, so continue with the next iteration of the loop
                continue;
            }
            else
            {
                perror("ERROR! "); // Print the error message specified by errno
                break;
            }
        }

        // if(isRaining) {
        //     cout<<"Omg! It's raining!!! 🌧"<<endl;
        // } else {
        //     cout<<"Yay! It's not raining!"<<endl;
        // }
    }

    cout << "Recieved instruction to halt program, closing now " << endl;
    close(runningFifoFd);
    pthread_exit(NULL);
}

void *checkAlerts(void *arg)
{
    int leaveWorkerId, deathWorkerId, promoteWorkerId;
    int workerLeaveAlertFd = open(workerLeaveAlert, O_RDWR | O_NONBLOCK); // Open pipe for reading and writing in non-blocking mode
    int workerDeathAlertFd = open(workerDeathAlert, O_RDWR | O_NONBLOCK);
    int workerPromoteAlertFd = open(workerPromotionAlert, O_RDWR | O_NONBLOCK);

    while (isRunning)
    {
        // cout<<"CHECKING ALERTS"<<endl;

        ssize_t leaveBytesRead = read(workerLeaveAlertFd, &leaveWorkerId, sizeof(leaveWorkerId));
        ssize_t deathBytesRead = read(workerDeathAlertFd, &deathWorkerId, sizeof(deathWorkerId));
        ssize_t promoteBytesRead = read(workerPromoteAlertFd, &promoteWorkerId, sizeof(promoteWorkerId));

        // cout<<"LEAVE WORKER ID READ: "<<leaveWorkerId<<endl;

        if (deathBytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No data was read from the FIFO, so continue with the next iteration of the loop
            }
            else
            {
                perror("ERROR! "); // Print the error message specified by errno
                break;
            }
        }
        else
        {

            if (deathWorkerId > -1)
            {
                cout << "RIP! Worker " << deathWorkerId << " died!" << endl;
                for (int i = 0; i < workingWorkers.size(); i++)
                {
                    if (workingWorkers[i].workerId == deathWorkerId)
                    {
                        workingWorkers.erase(workingWorkers.begin() + i);
                        break;
                    }
                    // TODO @MUSA halt task which worker was assigned to
                }
                for (int i = 0; i < idleWorkers.size(); i++)
                {
                    if (idleWorkers[i].workerId == deathWorkerId)
                    {
                        idleWorkers.erase(idleWorkers.begin() + i);
                        break;
                    }
                }
                int writeback = -1;
                write(workerDeathAlertFd, &writeback, sizeof(writeback));
                updateWorkersLists();
            }
        }
        //  cout<<"LEAVE WORKER ID " <<leaveWorkerId<<endl;

        if (leaveBytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No data was read from the FIFO, so continue with the next iteration of the loop
            }
            else
            {
                perror("ERROR! "); // Print the error message specified by errno
                break;
            }
        }
        else
        {
            if (leaveWorkerId > -1)
            {
                // cout<<"LEAVE WORKER ID IN IF STMT " <<leaveWorkerId<<endl;
                int writeback = -2;
                cout << "Worker " << leaveWorkerId << " applied for leave" << endl;
                for (int i = 0; i < workingWorkers.size(); i++)
                {
                    if (workingWorkers[i].workerId == leaveWorkerId)
                    {
                        cout << "Leave application rejected because worker is assigned to a task. Pls complete task before leaving." << endl;
                        sleep(1); // sleep
                        writeback = -2;
                        break;
                    }
                }
                for (int i = 0; i < idleWorkers.size(); i++)
                {
                    if (idleWorkers[i].workerId == leaveWorkerId)
                    {
                        cout << "Leave application accepted, you may go on leave. Bye!" << endl;
                        sleep(1); // sleep
                        writeback = -1;
                        backupWorkers.push_back(idleWorkers[i]);
                        idleWorkers.erase(idleWorkers.begin() + i);
                        break;
                    }
                }
                // -1 indicates accepted, -2 indicates rejected
                write(workerLeaveAlertFd, &writeback, sizeof(writeback));
                updateWorkersLists();
            }
        }

        if (promoteBytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No data was read from the FIFO, so continue with the next iteration of the loop
            }
            else
            {
                perror("ERROR! "); // Print the error message specified by errno
                break;
            }
        }
        else
        {
            // negative worker id == demote the worker with that id
            if (promoteWorkerId < 0)
            {
                cout << "Worker " << promoteWorkerId << " demoted " << endl;
                for (int i = 0; i < idleWorkers.size(); i++)
                {
                    if (idleWorkers[i].workerId == (-1) * promoteWorkerId)
                    {
                        idleWorkers[i].skillLevel--;
                        break;
                    }
                }
                for (int i = 0; i < workingWorkers.size(); i++)
                {
                    if (workingWorkers[i].workerId == (-1) * promoteWorkerId)
                    {
                        workingWorkers[i].skillLevel--;
                        break;
                    }
                }
                int writeback = 999;
                write(workerPromoteAlertFd, &writeback, sizeof(writeback));
                updateWorkersLists();

                // positive worker id == promote the worker with that id
            }
            else if (promoteWorkerId > 0)
            {
                cout << "Worker " << promoteWorkerId << " promoted " << endl;
                for (int i = 0; i < idleWorkers.size(); i++)
                {
                    if (idleWorkers[i].workerId == promoteWorkerId)
                    {
                        idleWorkers[i].skillLevel++;
                        break;
                    }
                }
                for (int i = 0; i < workingWorkers.size(); i++)
                {
                    if (workingWorkers[i].workerId == promoteWorkerId)
                    {
                        workingWorkers[i].skillLevel++;
                        break;
                    }
                }
                int writeback = 999;
                write(workerPromoteAlertFd, &writeback, sizeof(writeback));
                updateWorkersLists();
            }
            else
            {
                // do nothing!
            }
        }
    }

    // closinf pipes because ending program
    close(workerLeaveAlertFd);
    close(workerDeathAlertFd);
    pthread_exit(NULL);
}

int main()
{
    // Initialize the semaphores
    for (int i = 0; i < 3; i++)
    {
        sem_init(&empty[i], 0, MAX_CAPACITY);
        sem_init(&full[i], 0, 0);
    }

    srand(time(NULL));

    cout << "Generating workers" << endl;
    for (int i = 0; i < 25; i++)
    {
        idleWorkers.push_back(workerGenerator.generateWorker());
    }
    for (int i = 0; i < 25; i++)
    {
        backupWorkers.push_back(workerGenerator.generateWorker());
    }
    updateWorkersLists();

    const int EXECUTERS = 1; // number of threads for executing tasks
    pthread_t supply, degrade, fatigue, createTask, executeTasks[EXECUTERS], checkRunningStatus, checkAlertsStatus;
    pthread_create(&checkRunningStatus, NULL, checkRunning, NULL);
    pthread_create(&checkAlertsStatus, NULL, checkAlerts, NULL);
    pthread_create(&supply, NULL, supplyFactory, NULL);
    pthread_create(&degrade, NULL, materialDegredation, NULL);
    pthread_create(&fatigue, NULL, workerFatigue, NULL);
    pthread_create(&createTask, NULL, taskCreation, NULL);

    for (int i = 0; i < EXECUTERS; i++)
    { // create threads for executing tasks
        pthread_create(&executeTasks[i], NULL, tasksExecution, NULL);
    }

    pthread_join(checkRunningStatus, NULL);
    pthread_join(checkAlertsStatus, NULL);
    pthread_join(supply, NULL);
    pthread_join(degrade, NULL);
    pthread_join(fatigue, NULL);
    pthread_join(createTask, NULL);

    for (int i = 0; i < EXECUTERS; i++)
    {
        pthread_join(executeTasks[i], NULL);
    }

    // remove fifo
    unlink(runningFifoPath);
    unlink(rainingFifoPath);

    return 0;
}