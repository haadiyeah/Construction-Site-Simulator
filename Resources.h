#pragma once
#include <string>
using namespace std;

struct Resource {
    string type;
    float quality; //for degradation; initially 100%
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

