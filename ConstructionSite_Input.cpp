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
const char * runningFifoPath = "/tmp/isRunningFifo";
const char * rainingFifoPath = "/tmp/isRainingFifo";
const char * idleWorkersFifoPath = "/tmp/idleWorkersFifo";
const char * workingWorkersFifoPath = "/tmp/workingWorkersFifo";

vector<Worker> idleWorkers;
vector<Worker> workingWorkers;


void getWorkersLists() {
    char buffer[4096];

    //-----------------------Idle workers----------------------------------
    int idleWorkersFifoFd = open(idleWorkersFifoPath, O_RDONLY | O_NONBLOCK ); // Open pipe for reading
    if (idleWorkersFifoFd == -1) {
        cerr << "Failed to open " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }

    ssize_t bytesRead = read(idleWorkersFifoFd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        cerr << "Failed to read from " << idleWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }
    
    vector<string> serializedIdleWorkers = split(string(buffer), '\n');
    buffer[bytesRead] = '\0'; // Null-terminate the buffer
    if (bytesRead == sizeof(buffer) - 1 && buffer[bytesRead - 1] != '\0') {
        cerr << "Buffer too small when reading from " << idleWorkersFifoPath << endl;
    }
    idleWorkers = deserializeWorkers(serializedIdleWorkers);
  
    //-----------------------Working workers----------------------------------
    int workingWorkersFifoFd = open(workingWorkersFifoPath, O_RDONLY| O_NONBLOCK);
    if (workingWorkersFifoFd == -1) {
        cerr << "Failed to open " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }
       ssize_t bytesRead2 = read(workingWorkersFifoFd, buffer, sizeof(buffer) - 1);
    if (bytesRead2 == -1) {
        cerr << "Failed to read from " << workingWorkersFifoPath << ": " << strerror(errno) << endl;
        return;
    }

    vector<string> serializedWorkingWorkers = split(string(buffer), '\n');
    buffer[bytesRead2] = '\0'; // Null-terminate the buffer
    if (bytesRead2 == sizeof(buffer) - 1 && buffer[bytesRead2 - 1] != '\0') {
        cerr << "Buffer too small when reading from " << workingWorkersFifoPath << endl;
    }
    workingWorkers = deserializeWorkers(serializedWorkingWorkers);
}


int main() {
    int choice=-1;
    // Create the named pipe

    mkfifo(runningFifoPath, 0666);
    mkfifo(rainingFifoPath, 0666);
    mkfifo(idleWorkersFifoPath, 0666);
    mkfifo(workingWorkersFifoPath, 0666);
    getWorkersLists();

    do {
    cout<<" --- Construction Site --- "<<endl;
    cout<<" Please select an option from the menu! "<<endl;
    cout<<" 1. View worker lists \n 2. Worker leave/sickness/death \n 3. Worker promotion/demotion "<<
         "\n 4. Update Weather condition \n 0. Stop program "<<endl;
    
    cin>>choice;

    while( (choice < 0 || choice > 4)) {
        cout<<"Please enter a valid choice!"<<endl;
        cin.clear();
        cin.ignore(100,'\n');
    }

    switch(choice) {
        case 1: {
            cout<<"Worker Lists"<<endl;
            getWorkersLists();

            cout<<"Idle workers:\n------------";
            for(int i=0; i<idleWorkers.size(); i++) {
                cout<<"\nWorker id: "<<idleWorkers[i].workerId<<endl;
            }
            cout<<"------------"<<endl;
            cout<<"Working workers:\n------------";
            for(int i=0; i<workingWorkers.size(); i++) {
                cout<<"\nWorker id: "<<workingWorkers[i].workerId<<endl;
            }
            cout<<"------------"<<endl;
            break;
        }
        case 2: {
            cout<<"Worker death"<<endl;
            //if leave/sickness, select worker to make sick/leave
            //update accordingly, make sure worker comes back after certain amount of time
            //if death, similar as above but worker gone forever
            break;
        }
        case 3: {
            cout<<"Worker promotion"<<endl;
            //increase worker skill level and update queues accordingly
            //worker would have to be reassigned 
            break;
        }
        case 4: {
            int ans;
            cout<<"Weather condition: Is it raining? (Enter 1=Yes, 0=No)"<<endl;
            cin>>ans;
            if(ans == 1) {
                cout<<"OK, It's raining."<<endl;
                isRaining = true;
            } else {
                isRaining = false;
            }
            int fifoFd = open(rainingFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
            write(fifoFd, &isRaining, sizeof(isRaining));
            close(fifoFd);
            sleep(1);
            break;
        }
        case 0: {
            cout<<"Stopping program..."<<endl;
            isRunning = false;
            int fifoFd = open(runningFifoPath, O_WRONLY | O_NONBLOCK); // Open pipe for writing in non-blocking mode
            write(fifoFd, &isRunning, sizeof(isRunning));
            sleep(1); //delay needed else the other program is unable to read correctly
            break;
        }
        default: {
            cout<<"Please enter a valid choice!"<<endl;
            break;
        }
    }
    } while (choice != 0);

    cout<<"Program stopped!"<<endl;
    unlink(runningFifoPath); //note: removes the name of the FIFO from the filesystem. It doesn't actually destroy the FIFO until all processes have closed it.
    unlink(rainingFifoPath);
}