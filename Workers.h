#pragma once
#include <string>
#include <vector>
using namespace std;

struct Worker {
    int workerId;
    vector<string> skillSet;
    int skillLevel; //1 (High), 2 (Medium), 3 (Low)
    float fatigue; //initially 100
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
        int skillSet = rand() % 3;

        for(int i=0;i<3;i++) {
            w.skillSet.push_back(skillSets[skillSet][i]);
        }

        w.skillLevel = skillLevel;
        return w;
    }

};


//each skillset has 3 skills, one of each priority type.
int WorkerGenerator::workerId = 0;
string WorkerGenerator::skillSets[3][3] = { { "Landscaping", "Scaffolding", "Roofing"}, 
                                            {"Decorating", "Bricklaying", "Foundation Laying"}, 
                                            {"Painting", "Cement Mixing", "Structural Framing"} };
