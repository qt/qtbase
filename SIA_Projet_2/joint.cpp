#include "joint.h"
#include <algorithm> 
#include <cctype>
#include <locale>
//#include <QtGui/QMatrix4x4>

#include <QMatrix4x4>
#include <cmath>

using namespace std;

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

vector<double> getValuesFromLine(string line, char delimiter) {
	/**
	 * Parses a line with a given delimiter
	*/
	vector<double> values;
	std::setlocale(LC_ALL, "en_US.UTF-8");
	while (line.find(delimiter)!=string::npos){
		size_t index = line.find(delimiter);
		values.push_back(stod(line.substr(0,index)));
		line.erase(0,index+1);
	}
	if (line.size()>0){
		values.push_back(stod(line));
	}
	return values;
}

vector<vector<double>> reverse2DVec(vector<vector<double>> v) {
	vector<vector<double>> v2;
	int lenfirst = v[0].size();
	for (int i = 0; i<lenfirst; ++i) {
		vector<double> vec;
		v2.push_back(vec);
	}
	for (vector<double> vec : v) {
		for (int i = 0; i< (int)vec.size(); i++) {
			v2[i].push_back(vec[i]);
		}
	}
	return v2;
}

size_t addValuesToJoints(Joint* jnt, vector<vector<double>> vec, size_t pos) {
	for (auto it = jnt->_dofs.begin(); it != jnt->_dofs.end(); it++) {
		it->_values.insert(it->_values.end(), vec[pos].begin(), vec[pos].end());
		++pos;
	}
	if (jnt->_children.size()>0) {
		for (Joint* j : jnt->_children) {
			pos = addValuesToJoints(j, vec, pos);
		}
	}
	return pos;

}

pair<pair<Joint*, vector<Joint*>>, pair<int, double>> Joint::createFromFile(std::string fileName) {
	Joint* root = NULL;
	cout << "Loading from " << fileName << endl;
	vector<vector<double>> motionValues;
	ifstream inputfile(fileName.data());
	int nFrames;
	double frameInterval;
	vector<Joint*> jntVec;
	if(inputfile.good()) {

		bool isRoot = false;
		bool backtracking = false;
		double offX = 0;
		double offY = 0;
		double offZ = 0;
		string name;
		Joint *parent = nullptr;
		Joint *currentJoint = NULL;
		bool motionPart = false;
		vector<double> values;

		while(!inputfile.eof()) {
			std::setlocale(LC_ALL, "en_US.UTF-8");
			
			string buf;	
			std::getline(inputfile, buf);
			ltrim(buf);
			int size = buf.size();

			//cout << buf << endl;

			//buf.replace(0, string::npos, '	', ' ');
			if(buf.find("HIERARCHY") != string::npos) {
				continue;
			}

			if (buf.find("ROOT") != string::npos) {
				buf.erase(size-1,1);
				name = buf.substr(5, string::npos);
				isRoot = true;
			}

			if (buf.find("JOINT") != string::npos) {
				buf.erase(size-1,1);
				name = buf.substr(6, string::npos);
				isRoot = false;
			}

			if (buf.find("End") != string::npos) {
				buf.erase(size-1,1);
				name = buf.substr(4, string::npos);
				isRoot = false;
			}
			
			if (buf.find("OFFSET") != string::npos) {
				buf = buf.substr(buf.find(" ") + 1, string::npos);
				int index = buf.find(" ");
				offX = stod(buf.substr(0, index));
				buf = buf.substr(index + 1, string::npos);
				index = buf.find(" ");
				offY = stod(buf.substr(0, index));
				buf = buf.substr(index + 1, string::npos);
				index = buf.find(" ");
				offZ = stod(buf.substr(0, index));
				currentJoint = Joint::create(name, offX, offY, offZ, parent);
				jntVec.push_back(currentJoint);
				if(isRoot){
					root = currentJoint;
				}
				parent = currentJoint;
			}			

			if (buf.find("}") != string::npos) {
				if(backtracking){
					currentJoint = currentJoint->parent;
					if(currentJoint != NULL) parent = currentJoint->parent;
				}
				backtracking = true;
			}else{
				backtracking = false;
			}

			if (buf.find("CHANNELS")!=string::npos) {
				buf = buf.substr(buf.find(' ')+3,string::npos);
				vector<string> channels = getChannels(buf, ' ');
				for (string c : channels) {
					AnimCurve ac;
					ac.name = c;
					currentJoint->_dofs.push_back(ac);
				}

			}
				
			if (buf.find("MOTION")!=string::npos) {
				motionPart = true;
			}

			if (motionPart && buf.find("Frames")!=string::npos) {
				size_t index = buf.find(' ');
				nFrames = stoi(buf.substr(index+1,  string::npos));
			}

			if (motionPart && buf.find("Frame Time")!=string::npos) {
				size_t index = buf.find(':');
				frameInterval = stod(buf.substr(index+2,string::npos));
			}

			string motions = "1234567890-";
			if (motionPart && motions.find(buf[0])!=string::npos) {
				// Checking if in motion part and starting by a number
				values = getValuesFromLine(buf, (char) 9);
				motionValues.push_back(values);
			}

		}
		inputfile.close();
	} else {
		std::cerr << "Failed to load the file " << fileName.data() << std::endl;
		fflush(stdout);
	}
	motionValues = reverse2DVec(motionValues);
	addValuesToJoints(root, motionValues, 0);
	cout << "file loaded" << endl;
	pair<int, double> p{nFrames, frameInterval};
	pair<Joint*, vector<Joint*>> p2{root, jntVec};
	pair<pair<Joint*, vector<Joint*>>, pair<int, double>> data{p2, p};
	return data;
}

float dTR(float angle){
	return angle * M_PI / 180;
}

void Joint::animate(int iframe) 
{
	// Update dofs :
	_curTx = 0; _curTy = 0; _curTz = 0;
	_curRx = 0; _curRy = 0; _curRz = 0;
	for (unsigned int idof = 0 ; idof < _dofs.size() ; idof++) {
		if(!_dofs[idof].name.compare("Xposition")) _curTx = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yposition")) _curTy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zposition")) _curTz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zrotation")) _curRz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yrotation")) _curRy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Xrotation")) _curRx = _dofs[idof]._values[iframe];
	}
	// std::cout<<_curTx<<" "<<_curTy<<" "<<_curTz<<" "<<_curRz<<" "<<_curRy<<" "<<_curRx<<std::endl;
	QMatrix4x4 scale;
	if (parent == nullptr){
		scale = QMatrix4x4(divider, 0, 0, 0, 0, divider, 0, 0, 0, 0, divider, 0, 0, 0, 0, 1);
	}
	QMatrix4x4 rotX(1, 0, 0, 0, 0, cos(dTR(_curRx)), -sin(dTR(_curRx)), 0, 0, sin(dTR(_curRx)), cos(dTR(_curRx)), 0, 0, 0, 0, 1);
	QMatrix4x4 rotY(cos(dTR(_curRy)), 0, sin(dTR(_curRy)), 0, 0, 1, 0, 0, -sin(dTR(_curRy)), 0, cos(dTR(_curRy)), 0, 0, 0, 0, 1);
	QMatrix4x4 rotZ(cos(dTR(_curRz)), -sin(dTR(_curRz)), 0, 0, sin(dTR(_curRz)), cos(dTR(_curRz)), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	QVector4D trans(_offX + _curTx, _offY + _curTy, _offZ + _curTz, 1/divider);
	trans = trans * divider;
	*_transform = scale * rotZ * rotY * rotX;
	_transform->setColumn(3, trans);
	
	// for(int i = 0 ; i < 4 ; i++){
	// 	qDebug()<<_transform->row(i).x()<<_transform->row(i).y()<<_transform->row(i).z()<<_transform->row(i).w();
	// }

	// if(parent != NULL){
	// 	*_transform = *(parent->_transform) * *_transform;
	// }
	// Animate children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->animate(iframe);
	}
}

void Joint::nbDofs() {
	if (_dofs.empty()) return;

	int nbDofsR = -1;

	// TODO :
	cout << _name << " : " << nbDofsR << " degree(s) of freedom in rotation\n";

	// Propagate to children :
	for (int ichild = 0 ; ichild < (int)_children.size() ; ichild++) {
		_children[ichild]->nbDofs();
	}

}
