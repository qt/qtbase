
#include <vector>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

class AnimCurve {
    public:
        AnimCurve(){};
        ~AnimCurve() {
            _values.clear();
        }
    public:
        string name;
        vector<double> _values;
};

class Joint {
    public:
        vector<AnimCurve> _dofs;
        vector<Joint> _children;
        string name;

    public:
        Joint(){};
        ~Joint(){
            _dofs.clear();
            _children.clear();
        }
};

Joint* makeTree(){
    Joint* root = new Joint();
    root->name = "root";
    AnimCurve* curve1 = new AnimCurve();
    curve1->name = "root_c1";
    AnimCurve* curve2 = new AnimCurve();
    curve2->name = "root_c2";
    root->_dofs.push_back(*curve1);
    root->_dofs.push_back(*curve2);
    Joint* c1 = new Joint();
    Joint* c2 = new Joint();
    c1->name = "child 1";
    c2->name = "child 2";
    root->_children.push_back(*c1);
    root->_children.push_back(*c2);
    return root;

}

void printJoint(Joint* jnt){
    cout << jnt->name << endl;
    cout << "Children names : ";
    for (int i = 0; i < jnt->_children.size(); ++i){
        cout << jnt->_children[i].name;
    }
    cout << endl;
}

void printVect(vector<double> vec){
    cout << "[ " ;
    for (int i = 0; i < vec.size(); ++i) {
        cout << vec[i] << " ";
    }
    cout << ']' << endl;
}

unsigned int addAnimCurve(vector<double> valuesVector, Joint* jnt, unsigned int startIndex) {
    unsigned int start = startIndex;
    for (unsigned int i = 0; i < jnt->_dofs.size(); ++i){
        jnt->_dofs[i]._values.push_back(valuesVector[start]);
        start++;
    }
    for (unsigned int ichild = 0; ichild < jnt->_children.size(); ++ichild) {
        start = addAnimCurve(valuesVector, jnt->_children[ichild], start);
    }
    return start;
}

int main(){
    return 0;
}