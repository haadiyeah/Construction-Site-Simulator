#pragma once
#include <string>
#include <vector>
#include <sstream>
using namespace std;

struct Worker {
    int workerId;
    vector<string> skillSet;
    int skillLevel; //1 (High), 2 (Medium), 3 (Low)
    float fatigue; //initially 0
}; 

class WorkerGenerator {
    private:
    static int workerId;
    static string skillSets[3][3];
   
    public:
    static Worker generateWorker(int skillLevel=0) {
        if(skillLevel<1 || skillLevel>3) {
            skillLevel = rand() % 3 + 1;
        }

        Worker w;
        w.workerId = workerId++;
        w.fatigue = 0;
        int skillSet = rand() % 3;

        for(int i=0;i<3;i++) {
            w.skillSet.push_back(skillSets[skillSet][i]);
        }

        w.skillLevel = skillLevel;
        w.fatigue = 0;
        return w;
    }

};


//each skillset has 3 skills, one of each priority type.
int WorkerGenerator::workerId = 0;
string WorkerGenerator::skillSets[3][3] = { { "Landscaping", "Scaffolding", "Roofing"}, 
                                            {"Decorating", "Bricklaying", "Foundation-Laying"}, 
                                            {"Painting", "Cement-Mixing", "Structural-Framing"} };


//------------Print a worker
void printWorker(const Worker& worker) {
    cout << "Worker ID: " << worker.workerId << endl;
    cout << "Skill Level: " << worker.skillLevel << endl;
    cout << "Fatigue: " << worker.fatigue << endl;
    cout << "Skill Set: {";
    for (const string& skill : worker.skillSet) {
        cout << skill << " ";
    }
    cout <<" } "<< endl;
}

//------------Functions to serialize/deserialize worker objects-----------------
//serialize worker object to read/write to pipe
string serializeWorker(const Worker& worker) {
    ostringstream oss;
    oss << worker.workerId << " "
        << worker.skillLevel << " "
        << worker.fatigue << " "
        << worker.skillSet.size() << " ";
    for (const string& skill : worker.skillSet) {
        oss << skill << " ";
    }
    return oss.str();
}

vector<string> serializeWorkers(const vector<Worker>& workers) {
    vector<string> serializedWorkers;
    for (const Worker& worker : workers) {
        serializedWorkers.push_back(serializeWorker(worker));
    }
    return serializedWorkers;
}


Worker deserializeWorker(const string& serializedWorker) {
    istringstream iss(serializedWorker);
    Worker worker;
    size_t numSkills;
    iss >> worker.workerId >> worker.skillLevel >> worker.fatigue >> numSkills;
    worker.skillSet.resize(numSkills);
    for (size_t i = 0; i < numSkills; ++i) {
        iss >> worker.skillSet[i];
    }
    return worker;
}

vector<Worker> deserializeWorkers(const vector<string>& serializedWorkers) {
    vector<Worker> workers;
    for (const string& serializedWorker : serializedWorkers) {
        workers.push_back(deserializeWorker(serializedWorker));
    }
    return workers;
}