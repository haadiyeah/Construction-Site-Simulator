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
const int MAX_CAPACITY = 50; //max capacity for each type of resource
pthread_mutex_t bricksMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cementMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t toolsMutex = PTHREAD_MUTEX_INITIALIZER;

queue<Resource> bricks;
queue<Resource> cement;
queue<Resource> tools;

queue<Task> tasks;

void* supplyFactory(void* arg) {
    while (isRunning) {
        if(bricks.size() < MAX_CAPACITY) {
            pthread_mutex_lock(&bricksMutex);
            Resource brick;
            brick.type = "Brick";
            brick.quality = 100;
            cout<<"Supply factory: Produced 1 brick "<<endl;
            bricks.push(brick);
            pthread_mutex_unlock(&bricksMutex);
        }
        if(cement.size() < MAX_CAPACITY) {
            pthread_mutex_lock(&cementMutex);
            Resource c;
            c.type = "Cement";
            c.quality = 100;
            cout<<"Supply factory: Produced 1 cement "<<endl;
            cement.push(c);
            pthread_mutex_unlock(&cementMutex);
        }
        if(tools.size() < MAX_CAPACITY) {
            pthread_mutex_lock(&toolsMutex);
            Resource t;
            t.type = "Tool";
            t.quality = 100;
            cout<<"Supply factory: Produced 1 tool "<<endl;
            tools.push(t);
            pthread_mutex_unlock(&toolsMutex);
        }
    }
    pthread_exit(NULL);
}

void* materialDegredation(void* arg) {
    while(isRunning) {
        if(!bricks.empty()) {
            pthread_mutex_lock(&bricksMutex);
            Resource brick = bricks.front();
            bricks.pop();
            brick.quality -= 10;
            if(brick.quality > 0) {
                bricks.push(brick);
            }
            else {
                cout<<"Material Degredation: Brick quality is 0 and brick is destoryed "<<endl;
            }
            pthread_mutex_unlock(&bricksMutex);
        }
        if(!cement.empty()) {
            pthread_mutex_lock(&cementMutex);
            Resource c = cement.front();
            cement.pop();
            c.quality -= 10;
            if(c.quality > 0) {
                cement.push(c);
            }
            else {
                cout<<"Material Degredation: Cement quality is 0 and cement is destroyed"<<endl;
            }
            pthread_mutex_unlock(&cementMutex);
        }
        if(!tools.empty()) {
            pthread_mutex_lock(&toolsMutex);
            Resource t = tools.front();
            tools.pop();
            t.quality -= 10;
            if(t.quality > 0) {
                tools.push(t);
            }
            else {
                cout<<"Material Degredation: Tool quality is 0 and tool is destroyed"<<endl;
            }
            pthread_mutex_unlock(&toolsMutex);
        }
        //sleep(10); //degredation after every 10 seconds
    }
    pthread_exit(NULL);
}

int main() {

cout<<"Main"<<endl;
    pthread_t supplyFactoryThread;
    pthread_t materialDegredationThread;
    pthread_create(&supplyFactoryThread, NULL, supplyFactory, NULL);
    pthread_create(&materialDegredationThread, NULL, materialDegredation, NULL);

    pthread_join(supplyFactoryThread, NULL);
    pthread_join(materialDegredationThread, NULL);


    return 0;
}