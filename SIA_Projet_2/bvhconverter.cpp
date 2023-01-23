#include "bvhconverter.h"

LJI* branchLJI(std::vector<std::string> jointNames) {
    // creates a LJI and branch them one by one 
    LJI* root = new LJI();
    root->name = jointNames[0];
    LJI* curr = root;
    for (int i = 1; i < jointNames.size(); i++) {
        LJI* nouv = new LJI();
        nouv->name = jointNames[i];
        nouv->parent = curr;
        curr->children.push_back(nouv);
        curr = nouv;
    }
    return root;
}

LJI* setupJointHierarchy(){
    // Dummy function, best option as we don't parse a file yet
    std::vector<std::string> leftLeg {"ULEGL", "LLEGL", "FOOTL"};
    std::vector<std::string> rightLeg {"ULEGR", "LLEGR", "FOOTR"};
    std::vector<std::string> leftArm {"SHOUL", "UARML", "FARML","HANDL"};
    std::vector<std::string> rightArm {"SHOUR", "UARMR", "FARMR","HANDR"};
    LJI* llj = branchLJI(leftLeg);
    LJI* rlj = branchLJI(rightLeg);
    LJI* laj = branchLJI(leftArm);
    LJI* raj = branchLJI(rightArm);
    LJI* root = new LJI();
    root->name = "PELV";
    llj->parent = root;
    rlj->parent = root;
    root->children.push_back(llj);
    root->children.push_back(rlj);
    LJI* torso = new LJI();
    LJI* head = new LJI();
    head->name = "HEAD";
    head->parent = torso;
    torso->name = "STERN";
    torso->children.push_back(head);
    laj->parent=torso;
    raj->parent=torso;
    torso->children.push_back(laj);
    torso->children.push_back(raj);
    torso->parent=root;
    root->children.push_back(torso);
    return root;
}

std::string rightShiftTabs(int indent) {
    std::string s;
    for (int i = 0; i<indent; i++) {
        s+='\t';
    }
    return s;
}

void recursiveHierarchyWriting(int indent, LJI* info, std::ostream& os) {
    if (info->children.empty()) {
        os << rightShiftTabs(indent) << "End " << info->name << '\r';
        os << rightShiftTabs(indent) << "{\r";
        os << rightShiftTabs(indent+1) << "OFFSET 0.000000 0.000000 0.000000\r";
        os << rightShiftTabs(indent) << "}\r";
    } else {
        os << rightShiftTabs(indent);
        if (info->parent==NULL) {
            os << "ROOT " << info->name << '\r';
            os << rightShiftTabs(indent) << "{\r";
            os << rightShiftTabs(indent+1) << "OFFSET 0.000000 0.000000 0.000000\r";
            os << rightShiftTabs(indent+1) << "CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation\r";
        } else {
            os << "JOINT " << info->name << '\r';
            os << rightShiftTabs(indent) << "{\r";
            os << rightShiftTabs(indent+1) << "OFFSET 0.000000 0.000000 0.000000\r";
            os << rightShiftTabs(indent+1) << "CHANNELS 3 Zrotation Yrotation Xrotation\r";
        }
        for (LJI* child : info->children) {
            recursiveHierarchyWriting(indent+1,child, os);
        }
        os << rightShiftTabs(indent) << "}\r";
    }
}

void writeMotion(LJI* root, std::ostream& os) {
    for (size_t i = 1; i<root->rotations.size(); ++i) {
        os << "0.000 0.000 0.000 ";
        recursiveRotationsWriting(i, root, os);
    }
}

void recursiveRotationsWriting(size_t index, LJI* joint, std::ostream& os) {
    QQuaternion qdiff = joint->rotations[index] - joint->rotations[0];
    // calcul 
    QVector3D vec(0,0,0);
}

void writeToFile(std::string filename) {
    std::ofstream os(filename);
    os << "HIERARCHY\r";
    LJI* root = setupJointHierarchy();
    recursiveHierarchyWriting(0,root,os);
    os.close();
}

int main(){
    writeToFile("sample.txt");
    return 0;
}