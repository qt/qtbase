#ifndef SUPERPARSER_H
#define SUPERPARSER_H

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <typeinfo>
#include <fstream>
#include <map>
#include <QQuaternion>
#include <QVector3D>
#include <QVector2D>
#include <locale>
#include <algorithm>

struct fileInfo {
    std::string filename;
    std::string jointCode;
    int id;
};

struct VertexData {
    QVector3D position;
    QVector2D texCoord;
};

class JointData {
    public:
        JointData(){};
        ~JointData(){
            rotations.clear();
            accelerations.clear();
            filenames.clear();
        }

    public:
        std::string name;
        std::vector<QQuaternion> rotations;
        std::vector<QVector3D> accelerations;
        std::vector<std::string> filenames;
        
};

std::vector<std::string> multiParser(std::string line, std::string delimiters);

std::vector<fileInfo> parseDirectory(std::string directory);

std::map<std::string, std::string> mapJointsToFilenames();

std::vector<std::string> getChannels(std::string line, char delimiter);

void lineLengthTest();

void printMap(std::map<std::string, std::string> mapping);

void mapPrintTest();

void parseTest();

void parseDirectoryTest();

void printFiles();

void parseXsensFilename(std::string filename, JointData* jd);

std::vector<JointData*> extractData(std::string directory);

void parseFiletest();

std::pair<std::vector<VertexData>, std::vector<unsigned short>> parseVertex(std::string filename);

#endif