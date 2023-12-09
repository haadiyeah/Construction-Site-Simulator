#include <iostream>

using namespace std;

int main() {
    int choice=0;
    do {
    cout<<" --- Construction Site --- "<<endl;
    cout<<" Please select an option from the menu! "<<endl;
    cout<<" 1. Worker sickness/Leave \n 2. Worker death \n 3. Worker promotion "<<
         "\n 4. Weather condition \n 0. Stop program "<<endl;

    while(!cin>>choice && (choice < 0 || choice > 4)) {
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
            cout<<"Weather condition"<<endl;
            break;
        }
        case 0: {
            cout<<"Stopping program..."<<endl;
            //isRunning becomes false, use pipes for this purpose.
            break;
        }
        default: {
            cout<<"Please enter a valid choice!"<<endl;
            break;
        }
    }
    } while (choice != 0);
}