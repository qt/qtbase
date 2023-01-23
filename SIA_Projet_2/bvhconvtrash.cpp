#include "bvhconverter.h"


std::map<std::string, QVector3D> m{{"FOOTR",QVector3D(0.000004,-3.481768,22.994136)}};

void printMap(std::map<std::string, QVector3D> mapping) {
    for (auto& t: mapping) {
        std::cout << " ( " << t.first << " , " << t.second.x() << " "<< t.second.y() << " "<< t.second.z() << " " << " ) " << std::endl;
    }
}

class Info {
    public:
        Info* parent = NULL;
        std::string data;
        std::vector<Info*> children;

    public:
        Info(){};
        ~Info(){};
};

Info* setupTest(){
    Info* root = new Info();
    root->data = "root";
    Info* curr = root;
    for (int i = 1; i<4; i++) {
        Info* nouv = new Info();
        nouv->data = std::to_string(i);
        nouv->parent = curr;
        curr->children.push_back(nouv);
    }
    curr = root->children[0];
    for (int i = 10; i < 15; i++) {
        Info* inf = new Info();
        inf->data = std::to_string(i);
        inf->parent = curr;
        curr->children.push_back(inf);
    }
    return root;
}

void printInfo(Info* root) {
    Info* curr = root;
    for (Info* i : root->children) {
        std::cout << i->data << std::endl;
    }
}

std::string rightShiftTabs(int indent) {
    std::string s;
    for (int i = 0; i<indent;i++) {
        s+='\t';
    }
    return s;
}

void recursiveHierarchyWriting(int indent, Info* infos, std::ostream& os) {
    if (infos->children.empty()) {
        os << rightShiftTabs(indent) << "End " << infos->data << '\n';
        os << rightShiftTabs(indent) << "{\n";
        // TODO : add offset for end joint
        os << rightShiftTabs(indent) << "}\n";
    } else {
        if (infos->parent==NULL) {
            // root
            os << rightShiftTabs(indent) << "ROOT " << infos->data << '\n';
        } else {
            os << rightShiftTabs(indent) << "JOINT " << infos->data << '\n';
        }
        os << rightShiftTabs(indent) << "{\n";
        // TODO : add Channels et offsets
        for (Info* child : infos->children){
            recursiveHierarchyWriting(indent+1, child, os);
        }
        os << rightShiftTabs(indent) << "}\n";
    }
}

// std::string recursiveRotations(std::string out, Info* info) {
//     out+= info->data;
//     out += " ";
//     if (info->child!=NULL) {
//         out = recursiveRotations(out, info->child);
//     }
//     return out;
// }

void motionWriting(std::ostream& os, Info* info){
    /** TODO write lines 
     *  for i in range(len(infos->rotations)), do
     *  ajouter les infos translations de root (3 fos 0.000) puis récursion pour les rotations (en faisant le calcul à la volée)
     *  os << line (en oubliant pas le saut de ligne)
    */
}

void writeToFile(std::string filename) {
    std::ofstream os(filename);
    os << "HIERARCHY" << '\n';
    Info* info = setupTest();
    recursiveHierarchyWriting(0, info, os);
    os << '\n';
    os << "MOTION\n";
    os << "Frames: " << '\n';
    os << "Frame Time: 0.016667\n"; // Fixed bcz 60Hz refresh rate
    motionWriting(os, info);
    os.close();
    std::cout << "Recursive string addition : \n";
    std::string s = "";
    // s = recursiveRotations(s, info);
    std::cout << s << std::endl; 
}


int main() {
    //writeToFile("testbvh.txt");
    printMap(m);
    return 0;
}