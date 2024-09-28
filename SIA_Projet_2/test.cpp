#include "joint.h"

using namespace std;

void vectorPrint(vector<double> v){
    for (double n : v) {
        cout << n << " , ";
    }
    cout << endl;
}

void printChildren(Joint *jnt){
    if(jnt != NULL){
        Joint *parent = jnt->parent;
        std::cout<<"=========="<<std::endl;
        while(parent != NULL){
            std::cout << parent->_name << std::endl;
            parent = parent->parent;
        }
        std::cout << jnt->_name << std::endl;
        for(Joint *child : jnt->_children){
            printChildren(child);
        }
    }
}

void printMotion(Joint *jnt){
    vectorPrint(jnt->_dofs[0]._values);
}

int main(){
    Joint *root = Joint::createFromFile("walk1.bvh");
    // printChildren(root);
    cout << "Alors : " << root->_dofs.size() << endl;
    for (AnimCurve ac : root->_dofs) {
        cout << ac._values.size() << endl;
    }
    printMotion(root);
}