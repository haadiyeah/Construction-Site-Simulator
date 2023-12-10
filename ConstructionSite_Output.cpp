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
const char * idleWorkersFifoPath = "/tmp/idleWorkersFifo";
const char * workingWorkersFifoPath = "/tmp/workingWorkersFifo";

const int MAX_CAPACITY = 50; // max capacity for each type of resource
TaskGenerator taskGenerator;
TasksScheduler tasksScheduler;
WorkerGenerator workerGenerator;

pthread_mutex_t materialsMutex[3] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_mutex_t backupWorkerMutex = PTHREAD_MUTEX_INITIALIZER;
vector<vector<Resource>> materials(3); // 0 - bricks, 1 - cement, 2 - tools
vector<Worker> backupWorkers;          // virtual workers
vector<Worker> idleWorkers;
vector<Worker> workingWorkers;

void *supplyFactory(void *arg)
{
    while (isRunning)
    {
        for (int i = 0; i < 3; i++)
        {
            if (materials[i].size() < MAX_CAPACITY)
            {
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
                pthread_mutex_unlock(&materialsMutex[i]);
            }
        }
        sleep(5); // supply after every 5 seconds
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
                cout << "Material Degredation: " << r.type << " quality is " << r.quality << endl;
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
            return workingWorkers[i];
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

void *tasksExecution(void *arg) //
{
    cout << "Tasks Execution: Tasks execution started" << endl;

    while (isRunning)
    {
        pipe(parentToChild);
        pipe(childToParent);

        // Task task = tasksScheduler.getTask(isRaining, materials, idleWorkers);
        Task task = taskGenerator.generateTask();
        int initialTime = task.time;
        cout << "Tasks Execution: Task " << task.taskName << " is being executed" << endl;

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

                tasksScheduler.scheduleTask(task);  // change to aadd to halted tasks
            }
            else
            {
                cout << "Task " << task.taskName << " completed!" << endl;
                // release workers

                for (int i = 0; i < task.assignedWorkers.size(); i++)
                {
                    pthread_mutex_trylock(&backupWorkerMutex);
                    Worker worker = getWorkingWorker(task.assignedWorkers[i]);
                    worker.fatigue += (initialTime - task.time) * 5;    // fatigue increases by 5 for every second of work
                    backupWorkers.push_back(worker);
                    pthread_mutex_unlock(&backupWorkerMutex);

                    cout << "Worker " << worker.workerId << " is now going to backup, with fatigue: " << worker.fatigue << endl;
                }
            }
        }
        else
        {
            cout << "Error" << endl;
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

//function to update worker lists in pipe in order to be read by the other process
void updateWorkersLists(){
    int idleWorkersFifoFd = open(idleWorkersFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
    if (idleWorkersFifoFd == -1) {
        cerr << "Failed to open " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
       // return;
    }

    int workingWorkersFifoFd = open(workingWorkersFifoPath, O_WRONLY | O_NONBLOCK);
    if (workingWorkersFifoFd == -1) {
        cerr << "Failed to open " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
       // return;
    }

    vector<string> serializedIdleWorkers = serializeWorkers(idleWorkers);
    vector<string> serializedWorkingWorkers = serializeWorkers(workingWorkers);

    ostringstream ossIdle;
    ostringstream ossWorking;
    for (const string& serializedWorker : serializedIdleWorkers) {
        ossIdle << serializedWorker << "\n";
    }
    for(const string& serializedWorker : serializedWorkingWorkers){
        ossWorking << serializedWorker << "\n";
    }

    string idleOutput = ossIdle.str();
    string workingOutput = ossWorking.str();

    ssize_t written = write(idleWorkersFifoFd, idleOutput.c_str(), idleOutput.length());
    if (written == -1) {
        cerr << "Failed to write to " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
    } else if (written < idleOutput.length()) {
        cerr << "Buffer too small when writing to " << idleWorkersFifoPath << endl;
    }

    ssize_t written2 = write(workingWorkersFifoFd, workingOutput.c_str(), workingOutput.length());
    if (written2 == -1) {
        cerr << "Failed to write to " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
    } else if (written2 < workingOutput.length()) {
        cerr << "Buffer too small when writing to " << workingWorkersFifoPath << endl;
    }
}

int main()
{
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
    pthread_t supply, degrade, fatigue, createTask, executeTasks[EXECUTERS], checkRunningStatus;
    pthread_create(&checkRunningStatus, NULL, checkRunning, NULL);
    pthread_create(&supply, NULL, supplyFactory, NULL);
    pthread_create(&degrade, NULL, materialDegredation, NULL);
    pthread_create(&fatigue, NULL, workerFatigue, NULL);
    // pthread_create(&createTask, NULL, taskCreation, NULL);

    for (int i = 0; i < EXECUTERS; i++)
    { // create threads for executing tasks
        pthread_create(&executeTasks[i], NULL, tasksExecution, NULL);
    }

    pthread_join(checkRunningStatus, NULL);
    pthread_join(supply, NULL);
    pthread_join(degrade, NULL);
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