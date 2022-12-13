#include "joint.h"

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

int main(){
    Joint *root = Joint::createFromFile("run1.bvh");
    printChildren(root);
}