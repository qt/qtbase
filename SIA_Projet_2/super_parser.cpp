#include "super_parser.h"

// To parse all Xsens data files, use the extractData function


std::vector<std::string> multiParser(std::string line, std::string delimiters) {
    std::vector<std::string> tokens;
    while (line.find_first_of(delimiters)!=std::string::npos) {
        size_t index = line.find_first_of(delimiters);
        tokens.push_back(line.substr(0,index));
        line.erase(0,index+1);
    }
    if (line.size()>0) {
        tokens.push_back(line);
    }
    return tokens;
}

std::vector<fileInfo> parseDirectory(std::string directory) {
    std::vector<fileInfo> filenameData;
    for (const auto& file : std::filesystem::directory_iterator(directory)) {
        std::string filename = std::string(file.path().c_str());
        if (filename.find("readme")==std::string::npos) {
            // not parsing the readme
            std::vector<std::string> vec = multiParser(filename,"/._-");
            fileInfo fi;
            fi.filename = filename;
            fi.jointCode = vec[6];
            fi.id = stoi(vec[4]);
            filenameData.push_back(fi);
        }
    }
    return filenameData;
}


std::map<std::string, std::string> mapJointsToFilenames() {
    std::string filename = "xsens_data/readme.txt";
    std::ifstream inputfile(filename.data());
    std::map<std::string, std::string> mapping;
    if (inputfile.good()) {
        
        while(!inputfile.eof()){
            std::string buf;
            std::getline(inputfile, buf);
            if (buf.find('-')==std::string::npos && buf.size()>2) {
                if (buf.rfind('\r')!=std::string::npos) {
                    size_t ind = buf.rfind('\r');
                    buf = buf.substr(0,ind);
                }
                std::vector<std::string> vec = multiParser(buf, " ");   
                mapping.insert({vec[0],vec[1]});
            }
        }
    } 
    return mapping;
}


std::vector<std::string> getChannels(std::string line, char delimiter) {
	std::vector<std::string> chans;
	while (line.find(delimiter)!=std::string::npos) {
		size_t index = line.find(delimiter);
		chans.push_back(line.substr(0,index));
		line.erase(0,index+1);
	}
    int size = line.size();
	if (size>0) {
		// erase \r
		line.erase(size-1,1);
		chans.push_back(line);
	}
	return chans;
}

void lineLengthTest(){
    //testing the line lengths :
    std::string filename = "xsens_data/readme.txt";
    std::ifstream inputfile(filename.data());
    if (inputfile.good()) {
        int i = 1;
        while(!inputfile.eof()) {
            std::string buf;
            std::getline(inputfile, buf);
            if (buf.find('\r')!=std::string::npos) {
                std::cout << "Trouvé ligne " << i <<std::endl;
            }
            i++;
        }
        inputfile.close();
    }
}

void printMap(std::map<std::string, std::string> mapping) {
    for (auto& t: mapping) {
        std::cout << " ( " << t.first << " , " << t.second << " ) " << std::endl;
    }
}

void mapPrintTest(){
    std::map<std::string, std::string> m = mapJointsToFilenames();
    printMap(m);
}

void parseTest() {
    std::string delimiters = "/._-";
    std::string filename = "xsens_data/MT_0120083A-005-000_00B43DEF.txt";
    std::vector<std::string> v = multiParser(filename, delimiters);
    for (std::string elem : v) {
        std::cout << elem << " | ";
    }
    std::cout << std::endl;
}

void parseDirectoryTest(){
    std::vector<fileInfo> vec = parseDirectory("xsens_data/");
    fileInfo fi = vec[0];
    std::cout << "filename : " << fi.filename << std::endl;
    std::cout << "joint ID : " << fi.jointCode << std::endl;
    std::cout << "Order : " << fi.id << std::endl;
}

void printFiles() {
    std::string directory = "xsens_data/";
    for (const auto& file : std::filesystem::directory_iterator(directory)) {
        std::string filename = std::string(file.path().c_str());
        std::vector<std::string> vec = multiParser(filename, "/._-");
        for (std::string elem : vec) {
            std::cout << elem << " | ";
        }
        std::cout << std::endl;
    }
}

void parseXsensFilename(std::string filename, JointData* jd) {
    std::setlocale(LC_ALL, "en_US.UTF-8");
    std::ifstream inputfile(filename.data());
    if (inputfile.good()) {
        std::string digits = "0123456789";
        while (!inputfile.eof()) {
            std::string buf;
            std::getline(inputfile, buf);
            if (digits.find(buf[0])!=std::string::npos) {
                // if the line starts with a digit, we parse it, else we don't do anything
                std::vector<std::string> info = multiParser(buf, " \t\r\n");
                QVector3D a(std::stof(info[1]),std::stof(info[2]),std::stof(info[3]));
                jd->accelerations.push_back(a);
                QQuaternion rot(std::stof(info[4]),std::stof(info[5]),std::stof(info[6]),std::stof(info[7]));
                jd->rotations.push_back(rot);
            }
        }
        inputfile.close();
    }
}

std::vector<JointData*> extractData(std::string directory) {
    std::vector<JointData*> output;
    std::vector<fileInfo> infoVec = parseDirectory(directory);
    std::map<std::string, std::string> jointMap = mapJointsToFilenames();
    for (auto& t : jointMap) {
        JointData* jd = new JointData();
        jd->name = t.second;
        output.push_back(jd);
    }
    for (fileInfo fi : infoVec) {
        std::string currJointId = fi.jointCode;
        std::string JointName = jointMap[currJointId];
        for (JointData* jd : output) {
            if (jd->name==JointName) {
                jd->filenames.push_back(fi.filename);
            }
        }
    }
    for (JointData* jd : output) {
        std::sort(jd->filenames.begin(),jd->filenames.end());
        std::cout << jd->name << std::endl;
        for (std::string fn : jd->filenames) {
            std::cout << fn << std::endl;
            parseXsensFilename(fn, jd);
        }
    }
    return output;
}

void parseFiletest() {
    std::string filename = "xsens_data/MT_0120083A-005-000_00B43DEF.txt";
    JointData* jd = new JointData();
    std::cout << "merde pas ici ?" << std::endl;
    parseXsensFilename(filename, jd);
    for (auto& a : jd->accelerations) {
        std::cout << a.x() << " , " << a.y() << " , " << a.z() << std::endl;
    }
}

std::pair<std::vector<VertexData>, std::vector<unsigned short>> parseVertex(std::string filename) {
    std::setlocale(LC_ALL, "en_US.UTF-8");
    std::vector<VertexData> vvd;
    std::vector<unsigned short> vindvec;
    std::ifstream inputfile(filename.data());
    bool amounts = true;
    if (inputfile.good()) {
        while (!inputfile.eof()) {
            std::string buf;
            std::getline(inputfile, buf);
            std::vector<std::string> parsedLine = multiParser(buf, " \r\t\n");
            if (parsedLine.size()==3) {
                if (amounts){
                    amounts = false;
                    continue;
                }
                // case adding to VertexData vec
                VertexData vd;
                QVector3D pos = QVector3D(std::stof(parsedLine[0]),std::stof(parsedLine[1]),std::stof(parsedLine[2]));
                vd.position = pos;
                QVector2D tc = QVector2D(0.0,0.0);
                vd.texCoord = tc;
                vvd.push_back(vd);
            }
            else if (parsedLine.size()==4) {
                // case adding indexes vec
                vindvec.push_back(std::stof(parsedLine[1]));
                vindvec.push_back(std::stof(parsedLine[2]));
                vindvec.push_back(std::stof(parsedLine[3]));
            }
        }
        inputfile.close();
    }
    std::pair<std::vector<VertexData>,std::vector<unsigned short>> p(vvd,vindvec);
    return p;
}

// int main(){
//     //mapPrintTest();
//     //printFiles();
//     //parseDirectoryTest();
//     //parseFiletest();
//     //extractData("xsens_data/");
//     std::pair<std::vector<VertexData>, std::vector<unsigned short>> p = parseVertex("skin.off");
//     for (int i : p.second) {
//         std::cout << i << std::endl;
//     }
//     return EXIT_SUCCESS;
// }