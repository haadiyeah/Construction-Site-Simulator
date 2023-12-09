#pragma once
#include <string>
using namespace std;

struct Resource {
    string type;
    float quality; //for degradation; initially 100%
};

struct Task{
    string taskName;
    int priority; //1 (High), 2 (Medium), 3 (Low)
    int time; //number to sleep 
    vector<Resource> resources; //resources required for the task

};

struct Worker {
    int workerId;
    string skill; //bricklayer, cement mixer, scaffolder
    int skillLevel; //1 (High), 2 (Medium), 3 (Low)
    
}; 

//queue of bricks 
//queue of cement
//queue of tools
//each queue will have a certain MAX_CAPACITY

//one thread will simulate quality degradation in the above resources

//supply factory will rpelenish reosurces(thread chalta rahe ga)
//producer consumer problem

//tasks: 3 main
//laying bricks, mixing cement, scaffolding

