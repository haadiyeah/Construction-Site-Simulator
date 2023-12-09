#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

bool isRunning = true;
bool isRaining = false;
const char * runningFifoPath = "/tmp/isRunningFifo";
const char * rainingFifoPath = "/tmp/isRainingFifo";


using namespace std;

int main() {
    int choice=-1;
    // Create the named pipe

    mkfifo(runningFifoPath, 0666);
    mkfifo(rainingFifoPath, 0666);

    do {
    cout<<" --- Construction Site --- "<<endl;
    cout<<" Please select an option from the menu! "<<endl;
    cout<<" 1. Worker sickness/Leave \n 2. Worker death \n 3. Worker promotion "<<
         "\n 4. Update Weather condition \n 0. Stop program "<<endl;
    
    cin>>choice;

    while( (choice < 0 || choice > 4)) {
        cout<<"Please enter a valid choice!"<<endl;
        cin.clear();
        cin.ignore(100,'\n');
    }

    switch(choice) {
        case 1: {
            cout<<"Here are the worker queues"<<endl;
            //display which worker is working on tasks, which are idle etc
            //Select worker to make sick/leave
            //update accordingly, make sure worker comes back after certain amount of time
            break;
        }
        case 2: {
            cout<<"Worker death"<<endl;
            //similar as above but worker gone forever
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