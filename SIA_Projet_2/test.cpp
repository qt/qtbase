#include "joint.h"

void printChildren(Joint *jnt){
    if(jnt != NULL){
        std::cout<<jnt->_name<<std::endl;
        for(Joint *child : jnt->_children){
            printChildren(child);
        }
    }
}

int main(){
    Joint *root = Joint::createFromFile("run1.bvh");
    printChildren(root);
}