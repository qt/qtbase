#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix3x3>
#include <cmath>

#include "super_parser.h"

class LJI {
    public:
        std::string name;
        LJI* parent=NULL;
        std::vector<LJI*> children;
        std::vector<QVector3D> accelerations;
        std::vector<QQuaternion> rotations;
        std::vector<std::string> channels;
        QVector3D offset;

    public:
        LJI(){};
        ~LJI(){};
        //std::string createBVHfromData(std::vector<JointData*> extractedData);
        //LJI* setupHierarchy(std::map<std::string, std::vector<std::string>> heir);
};


