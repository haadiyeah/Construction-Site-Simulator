#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <vector>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <cstring>

#include "Workers.h"
#include "Utils.h"

using namespace std;

bool isRunning = true;
bool isRaining = false;
const char *runningFifoPath = "/tmp/isRunningFifo";
const char *rainingFifoPath = "/tmp/isRainingFifo";
const char *idleWorkersFifoPath = "/tmp/idleWorkersFifo";
const char *workingWorkersFifoPath = "/tmp/workingWorkersFifo";
const char *workerLeaveAlert = "/tmp/workerLeaveAlert";
const char *workerDeathAlert = "/tmp/workerDeathAlert";
const char *workerPromotionAlert = "/tmp/workerPromotionAlert";

vector<Worker> idleWorkers;
vector<Worker> workingWorkers;

void initFifos()
{
    // open the workerLeaveAlert, workerDeathAlert fifos and write -1 to them to clear them,
    // else the other program will be unable to read from them
    int fifoFd = open(workerLeaveAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
    if (fifoFd == -1)
    {
        cerr << "Failed to open " << workerLeaveAlert << ": " << strerror(errno) << endl;
    }
    int reply = -1;
    write(fifoFd, &reply, sizeof(reply));
    close(fifoFd);

    fifoFd = open(workerDeathAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
    if (fifoFd == -1)
    {
        cerr << "Failed to open " << workerDeathAlert << ": " << strerror(errno) << endl;
    }
    write(fifoFd, &reply, sizeof(reply));
    close(fifoFd);

    //in promotion alert the default value is zero because -1 = demotion, +1 = promotion.
    int zero=0;
    fifoFd = open(workerPromotionAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
    if (fifoFd == -1)
    {
        cerr << "Failed to open " << workerPromotionAlert << ": " << strerror(errno) << endl;
    }
    write(fifoFd, &zero, sizeof(zero));
    close(fifoFd);

}

void getWorkersLists()
{
    //-----------------------Idle workers----------------------------------
    char buffer[255];
    int idleWorkersFifoFd = open(idleWorkersFifoPath, O_RDONLY | O_NONBLOCK); // Open pipe for reading
    if (idleWorkersFifoFd == -1)
    {
        cerr << "Failed to open " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }

    vector<string> serializedIdleWorkers;
    ssize_t bytesRead;
    while (((bytesRead = read(idleWorkersFifoFd, buffer, sizeof(buffer) - 1)) > 0) 
     && (serializedIdleWorkers.size() < 25)
    )
    {
        if (bytesRead == -1)
        {
            cerr << "Failed to read from " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
            return;
        }

        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        vector<string> chunk = split(string(buffer), '\n');
        serializedIdleWorkers.insert(serializedIdleWorkers.end(), chunk.begin(), chunk.end());
    }
    idleWorkers = deserializeWorkers(serializedIdleWorkers);

    //-----------------------Working workers----------------------------------
    char buffer2[255];
    int workingWorkersFifoFd = open(workingWorkersFifoPath, O_RDONLY | O_NONBLOCK);
    if (workingWorkersFifoFd == -1)
    {
        cerr << "Failed to open " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }
 
    vector<string> serializedWorkingWorkers;
    ssize_t bytesRead2;
    while (((bytesRead2 = read(workingWorkersFifoFd, buffer2, sizeof(buffer2) - 1)) > 0) 
    && (serializedWorkingWorkers.size() < 25)
    )
    {
        if (bytesRead2 == -1)
        {
            cerr << "Failed to read from " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
            return;
        }
        buffer2[bytesRead2] = '\0'; // Null-terminate the buffer
        vector<string> chunk = split(string(buffer2), '\n');
        serializedWorkingWorkers.insert(serializedWorkingWorkers.end(), chunk.begin(), chunk.end());
    }

    workingWorkers = deserializeWorkers(serializedWorkingWorkers);
}

//returns skill level of the found worker else,-1
int findWorkerAndPrint(int wid)
{
    for (int i = 0; i < idleWorkers.size(); i++)
    {
        if (idleWorkers[i].workerId == wid)
        {
            printWorker(idleWorkers[i]);
            return idleWorkers[i].skillLevel;
        }
    }
    for (int i = 0; i < workingWorkers.size(); i++)
    {
        if (workingWorkers[i].workerId == wid)
        {
            printWorker(workingWorkers[i]);
            return workingWorkers[i].skillLevel;
        }
    }
    cout<<"Worker not found"<<endl;
    return -1;
}

int main()
{
    int choice = -1;
    // creating the named pipes occurs here, main reason why u must run this file first before running the other one
    mkfifo(runningFifoPath, 0666);
    mkfifo(rainingFifoPath, 0666);
    mkfifo(idleWorkersFifoPath, 0666);
    mkfifo(workingWorkersFifoPath, 0666);
    mkfifo(workerLeaveAlert, 0666);
    mkfifo(workerDeathAlert, 0666);
    mkfifo(workerPromotionAlert, 0666);

    // initFifos();

    getWorkersLists();
   
    do
    {
        cout << "\n --- Welcome To Slave Simulator! --- " << endl;
        cout << " Please select an option from the menu! " << endl;
        cout << " 1. View worker lists \n 2. Worker leave/sickness/death \n 3. Worker promotion/demotion "
             << "\n 4. Update Weather condition \n 0. Stop program " << endl;

        cin >> choice;

        while ((choice < 0 || choice > 4))
        {
            cout << "Please enter a valid choice!" << endl;
            cin.clear();
            cin.ignore(100, '\n');
        }

        switch (choice)
        {
        case 1:
        {
            cout << "Worker Lists" << endl;
            try
            {
                getWorkersLists();
            }
            catch (const std::bad_alloc &e)
            {
                cerr << "Memory issue occurred. Please try again in a while." << endl;
            }

            cout << "Idle workers:\n------------";
            for (int i = 0; i < idleWorkers.size(); i++)
            {
                if (idleWorkers[i].workerId > 0 && idleWorkers[i].workerId < idleWorkers.size())
                    cout << "\n\t - Worker id: " << idleWorkers[i].workerId
                         << "\nSkill Level: " << idleWorkers[i].skillLevel << ", Fatigue: " << idleWorkers[i].fatigue
                         << "\nSkill set: {" << idleWorkers[i].skillSet[0] << ", " << idleWorkers[i].skillSet[1] << ", " << idleWorkers[i].skillSet[2] << "}" << endl;
            }
            cout << "------------" << endl;
            cout << "Working workers:\n------------";
            for (int i = 0; i < workingWorkers.size(); i++)
            {
                if(workingWorkers[i].workerId > 0 && workingWorkers[i].workerId < workingWorkers.size())
                    cout << "\nWorker id: " << workingWorkers[i].workerId 
                        << "\nSkill Level: "<< workingWorkers[i].skillLevel << ", Fatigue: " << workingWorkers[i].fatigue
                        << "\nSkill set: {" << workingWorkers[i].skillSet[0] << ", " << workingWorkers[i].skillSet[1] << ", " << workingWorkers[i].skillSet[2] << "}"<<endl;
            }
            cout << "------------" << endl;
            break;
        }
        case 2:
        {
            int wid, choice;
            cout << "Please enter the ID of the unfortunate worker " << endl;
            cin >> wid;
            if(findWorkerAndPrint(wid) == -1) {
                break;
            }
            cout << "What happened to him? \n(1) Leave/Sickness \n(2) Death " << endl;
            cin >> choice;
            if (choice == 1)
            {
                int fifoFd = open(workerLeaveAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
                if (fifoFd == -1)
                {
                    cerr << "Failed to open " << workerLeaveAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                write(fifoFd, &wid, sizeof(wid));
                close(fifoFd);
                sleep(1);
                int fifoFdReply = open(workerLeaveAlert, O_RDONLY); // Open pipe for readinge
                if (fifoFdReply == -1)
                {
                    cerr << "Failed to open " << workerLeaveAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                cout << "Sent leave request, awaiting response... " << endl;
                int reply;
                ssize_t bytesRead = read(fifoFdReply, &reply, sizeof(reply));
                if (bytesRead == -1)
                {
                    cerr << "Failed to read from " << workerLeaveAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                if (bytesRead != sizeof(reply))
                {
                    cerr << "Invalid data read from " << workerLeaveAlert << endl;
                    continue;
                }
                if (reply == -1)
                {
                    cout << "Worker " << wid << " leave application has been accepted " << endl;
                }
                else if (reply == -2)
                {
                    cout << "Worker " << wid << " leave application was rejected !" << endl;
                }
                else
                {
                    cout << "Invalid reply received!" << endl;
                }
            }
            else if (choice == 2)
            {
                int fifoFd = open(workerDeathAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
                if (fifoFd == -1)
                {
                    cerr << "Failed to open " << workerDeathAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                write(fifoFd, &wid, sizeof(wid));
                close(fifoFd);
                int fifoFdReply = open(workerDeathAlert, O_RDONLY); // Open pipe for reading
                if (fifoFdReply == -1)
                {
                    cerr << "Failed to open " << workerDeathAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                int reply;
                ssize_t bytesRead = read(fifoFdReply, &reply, sizeof(reply));
                if (bytesRead == -1)
                {
                    cerr << "Failed to read from " << workerDeathAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                if (bytesRead != sizeof(reply))
                {
                    cerr << "Invalid data read from " << workerDeathAlert << endl;
                    continue;
                }
                if (reply == -1)
                {
                    cout << "Worker " << wid << " has been unalived successfully. " << endl;
                }
                else
                {
                    cout << "Invalid reply received" << endl;
                }
            }
            else
            {
                cout << "Invalid choice!" << endl;
            }
            break;
        }
        case 3:
        {
            int choice;
            cout << "Worker promotion/demotion" << endl;
            cout << "Please enter the ID of the worker you want to promote/demote" << endl;
            int wid;
            cin >> wid;
            cout<<"Worker info: "<<endl;
            int returnedSkillLevl = findWorkerAndPrint(wid);
            if (returnedSkillLevl == -1) {
                break;
            }
            cout << "Please enter the choice: \n(1) Promote \n(-1) Demote" << endl;
            cin >> choice;
            if(choice != 1 && choice != -1) {
                cout << "Invalid choice!" << endl;
            } else if( (returnedSkillLevl + choice) < 1 || (returnedSkillLevl + choice) > 3 ) {
                cout<<" Invalid! cannot perform the desired action (promote/demote) at this skill level "<<endl;
            } else {
                int fifoFd = open(workerPromotionAlert, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
                if (fifoFd == -1)
                {
                    cerr << "Failed to open " << workerPromotionAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                int writeNum = wid*choice;
                write(fifoFd, &writeNum, sizeof(writeNum));
                close(fifoFd);
                sleep(1);
                int fifoFdReply = open(workerPromotionAlert, O_RDONLY); // Open pipe for reading
                if (fifoFdReply == -1)
                {
                    cerr << "Failed to open " << workerPromotionAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                int reply;
                ssize_t bytesRead = read(fifoFdReply, &reply, sizeof(reply));
                if (bytesRead == -1)
                {
                    cerr << "Failed to read from " << workerPromotionAlert << ": " << strerror(errno) << endl;
                    continue;
                }
                if (bytesRead != sizeof(reply))
                {
                    cerr << "Invalid data read from " << workerPromotionAlert << endl;
                    continue;
                }
                if (reply == 999)
                {
                    cout << "Worker " << wid << " has been promoted/demoted successfully. " << endl;
                }
                else
                {
                    cout << "Sorry! Invalid reply received" << endl;
                }

            }


            // increase worker skill level and update queues accordingly
            // worker would have to be reassigned
            break;
        }
        case 4:
        {
            int ans;
            cout << "Weather condition: Is it raining? (Enter 1=Yes, 0=No)" << endl;
            cin >> ans;
            if (ans == 1)
            {
                cout << "OK, It's raining." << endl;
                isRaining = true;
            }
            else
            {
                isRaining = false;
            }
            int fifoFd = open(rainingFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
            write(fifoFd, &isRaining, sizeof(isRaining));
            close(fifoFd);
            sleep(1);
            break;
        }
        case 0:
        {
            cout << "Stopping program..." << endl;
            isRunning = false;
            int fifoFd = open(runningFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
            write(fifoFd, &isRunning, sizeof(isRunning));
            sleep(1); // delay needed else the other program is unable to read correctly
            break;
        }
        default:
        {
            cout << "Please enter a valid choice!" << endl;
            break;
        }
        }
    } while (choice != 0);

    cout << "Program stopped!" << endl;
    unlink(runningFifoPath); // note: removes the name of the FIFO from the filesystem. It doesn't actually destroy the FIFO until all processes have closed it.
    unlink(rainingFifoPath);
    unlink(idleWorkersFifoPath);
    unlink(workingWorkersFifoPath);
    unlink(workerLeaveAlert);
    unlink(workerDeathAlert);
    return 0;
}