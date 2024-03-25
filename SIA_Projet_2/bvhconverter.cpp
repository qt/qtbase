#include "bvhconverter.h"

std::map<std::string, QVector3D> m {{"STERN",QVector3D(0.0,52.0,0.0)},
                                    {"HEAD", QVector3D(0.0, 19.0, 0.0)},
                                    {"PELV", QVector3D(0.0, 0.0, 0.0)},
                                    {"SHOUL", QVector3D(-20.0, 0.0, 0.0)},
                                    {"UARML", QVector3D(-25.0, 0.0, 0.0)},
                                    {"FARML", QVector3D(-27.0, 0.0, 0.0)},
                                    {"HANDL", QVector3D(-18.0, 0.0, 0.0)},
                                    {"SHOUR", QVector3D(20.0, 0.0, 0.0)},
                                    {"UARMR", QVector3D(25.0, 0.0, 0.0)},
                                    {"FARMR", QVector3D(27.0, 0.0, 0.0)},
                                    {"HANDR", QVector3D(18.0, 0.0, 0.0)},
                                    {"ULEGL", QVector3D(-56.0*sin(M_PI/12.0), -56*cos(M_PI/12.0), 0.0)},
                                    {"LLEGL", QVector3D(0.0, -46.0, 0.0)},
                                    {"FOOTL", QVector3D(0.0, 0.0, 21.0)},
                                    {"ULEGR", QVector3D(56.0*sin(M_PI/12.0), -56*cos(M_PI/12.0), 0.0)},
                                    {"LLEGR", QVector3D(0.0, -46.0, 0.0)},
                                    {"FOOTR", QVector3D(0.0, 0.0, 21.0)}};


LJI* branchLJI(std::vector<std::string> jointNames) {
    // creates a LJI and branch them one by one 
    LJI* root = new LJI();
    root->name = jointNames[0];
    root->offset = m[root->name];
    LJI* curr = root;
    for (int i = 1; i < jointNames.size(); i++) {
        LJI* nouv = new LJI();
        nouv->name = jointNames[i];
        nouv->parent = curr;
        nouv->offset = m[nouv->name];
        curr->children.push_back(nouv);
        curr = nouv;
    }
    return root;
}

void branchRotations(LJI* joint, std::vector<JointData*> jdv) {
    for (JointData* jd : jdv) {
        if (joint->name==jd->name) {
            joint->rotations=jd->rotations;
            joint->accelerations = jd->accelerations;
        }
    }
    if (!joint->children.empty()) {
        for (LJI* child : joint->children) {
            branchRotations(child, jdv);
        }
    }
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
    root->offset = m[root->name];
    LJI* torso = new LJI();
    LJI* head = new LJI();
    head->name = "HEAD";
    head->parent = torso;
    head->offset = m[head->name];
    torso->name = "STERN";
    torso->children.push_back(head);
    torso->offset = m[torso->name];
    laj->parent=torso;
    raj->parent=torso;
    torso->children.push_back(laj);
    torso->children.push_back(raj);
    torso->parent=root;
    root->children.push_back(torso);
    branchRotations(root, extractData("xsens_data/"));
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
        os << rightShiftTabs(indent) << "End " << info->name << '\n';
        os << rightShiftTabs(indent) << "{\n";
        os << rightShiftTabs(indent+1) << "OFFSET " << info->offset.x() << " " << info->offset.y() << " " << info->offset.z() << '\n';
        os << rightShiftTabs(indent) << "}\n";
    } else {
        os << rightShiftTabs(indent);
        if (info->parent==NULL) {
            os << "ROOT " << info->name << '\n';
            os << rightShiftTabs(indent) << "{\n";
            os << rightShiftTabs(indent+1) << "OFFSET " << info->offset.x() << " " << info->offset.y() << " " << info->offset.z() << '\n';
            os << rightShiftTabs(indent+1) << "CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation\n";
        } else {
            os << "JOINT " << info->name << '\n';
            os << rightShiftTabs(indent) << "{\n";
            os << rightShiftTabs(indent+1) << "OFFSET " << info->offset.x() << " " << info->offset.y() << " " << info->offset.z() << '\n';
            os << rightShiftTabs(indent+1) << "CHANNELS 3 Zrotation Yrotation Xrotation\n";
        }
        for (LJI* child : info->children) {
            recursiveHierarchyWriting(indent+1,child, os);
        }
        os << rightShiftTabs(indent) << "}\n";
    }
}


void recursiveRotationsWriting(size_t index, LJI* joint, std::ostream& os) {
    QVector3D vec = (joint->rotations[index]*joint->rotations[0].conjugate()).toEulerAngles();
    os << " " << vec.z() << " " << vec.y() << " " << vec.x();
    if (!joint->children.empty()) {
        for (LJI* child : joint->children) {
            recursiveRotationsWriting(index, child, os);
        }
    }
}

void writeMotion(LJI* root, std::ostream& os) {
    for (size_t i = 1; i<root->rotations.size(); ++i) {
        os << "0.000 0.000 0.000";
        recursiveRotationsWriting(i, root, os);
        os << '\n';
    }
}

void writeToFile(std::string filename) {
    std::ofstream os(filename);
    os << "HIERARCHY\n";
    LJI* root = setupJointHierarchy();
    recursiveHierarchyWriting(0,root,os);
    os << "MOTION\n";
    os << "Frames: " << root->rotations.size()-1 << '\n';
    os << "Frame Time: 0.0166667\n";
    writeMotion(root, os);
    os.close();
}

int main(){
    writeToFile("sample.bvh");
    return 0;
}