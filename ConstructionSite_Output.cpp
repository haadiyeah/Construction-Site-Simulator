#include <iostream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include "Resources.h"
#include "Tasks.h"


using namespace std;

bool isRunning = true;
const int MAX_CAPACITY =50; //max capacity for each type of resource

pthread_mutex_t materialsMutex[3] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
vector<vector<Resource>> materials(3);  // 0 - bricks, 1 - cement, 2 - tools

void* supplyFactory(void* arg) {
    while (isRunning) {
        for(int i = 0; i < 3; i++) {
            if(materials[i].size() < MAX_CAPACITY) {
                pthread_mutex_lock(&materialsMutex[i]);
                Resource r;
                switch(i) {
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
                cout<<"Supply factory: Produced 1 "<<r.type<<endl;
                materials[i].push_back(r);
                pthread_mutex_unlock(&materialsMutex[i]);
            }
        }
        sleep(5); // supply after every 5 seconds
    }
    pthread_exit(NULL);
}

void* materialDegredation(void* arg) { // degrades the first pushed resource of each type
    while(isRunning) {
        cout << "Material Degredation: Degredation started" << endl;
        for(int i = 0; i < 3; i++) {
            if(!materials[i].empty()) {
                pthread_mutex_lock(&materialsMutex[i]);
                Resource r = materials[i].front();
                // materials[i].pop_back(); // remove the first pushed resource
                r.quality -= 10;
                cout << "Material Degredation: " << r.type << " quality is " << r.quality << endl;
                if(r.quality > 0) {
                    // materials[i].push_back(r);
                }
                else {
                    switch(i) {
                        case 0:
                            cout<<"Material Degredation: Brick quality is 0 and brick is destroyed"<<endl;
                            break;
                        case 1:
                            cout<<"Material Degredation: Cement quality is 0 and cement is destroyed"<<endl;
                            break;
                        case 2:
                            cout<<"Material Degredation: Tool quality is 0 and tool is destroyed"<<endl;
                            break;
                    }
                }
                pthread_mutex_unlock(&materialsMutex[i]);
            }
        }
        sleep(8); //degredation after every 10 seconds
    }
    pthread_exit(NULL);
}

int main() {

cout<<"Main"<<endl;
    pthread_t supply, degrade;
    pthread_create(&supply, NULL, supplyFactory, NULL);
    pthread_create(&degrade, NULL, materialDegredation, NULL);

    pthread_join(supply, NULL);
    pthread_join(degrade, NULL);


    return 0;
}