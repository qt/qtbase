#include "joint.h"
#include <algorithm> 
#include <cctype>
#include <locale>
//#include <QtGui/QMatrix4x4>

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

vector<string> getChannels(string line, char delimiter) {
	vector<string> chans;
	while (line.find(delimiter)!=string::npos) {
		size_t index = line.find(delimiter);
		chans.push_back(line.substr(0,index));
		line.erase(0,index+1);
	}
	if (line.size()>0) {
		chans.push_back(line);
	}
	return chans;
}

vector<vector<double>> reverse2DVec(vector<vector<double>> v) {
	vector<vector<double>> v2;
	size_t lenfirst = v[0].size();
	for (int i = 0; i<lenfirst; ++i) {
		vector<double> vec;
		v2.push_back(vec);
	}
	for (vector<double> wallah : v) {
		for (int i = 0; i< wallah.size(); i++) {
			v2[i].push_back(wallah[i]);
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

Joint* Joint::createFromFile(std::string fileName) {
	Joint* root = NULL;
	cout << "Loading from " << fileName << endl;
	vector<vector<double>> motionValues;
	ifstream inputfile(fileName.data());
	if(inputfile.good()) {

		bool isRoot = false;
		bool backtracking = false;
		double offX = 0;
		double offY = 0;
		double offZ = 0;
		string name;
		Joint *parent = NULL;
		Joint *currentJoint = NULL;
		bool motionPart = false;
		vector<double> values;

		while(!inputfile.eof()) {
			string buf;	
			std::getline(inputfile, buf);
			ltrim(buf);

			//buf.replace(0, string::npos, '	', ' ');
			if(buf.find("HIERARCHY") != string::npos) {
				continue;
			}

			if (buf.find("ROOT") != string::npos) {
				name = buf.substr(5, string::npos);
				isRoot = true;
			}

			if (buf.find("JOINT") != string::npos) {
				name = buf.substr(6, string::npos);
				isRoot = false;
			}

			if (buf.find("End") != string::npos) {
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
				//cout << name << endl;
				if(isRoot){
					root = currentJoint;
				}
				// else{
				// 	cout << "-> " << parent->_name << endl;
				// }
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
				int azert = 0;
				for (string c : channels) {
					//cout << "Ajout de channel " << azert << endl;
					AnimCurve ac;
					ac.name = c;
					currentJoint->_dofs.push_back(ac);
				}

			}
				
			if (buf.find("MOTION")!=string::npos) {
				motionPart = true;
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
	return root;
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
	// Animate children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->animate(iframe);
	}
}

void Joint::nbDofs() {
	if (_dofs.empty()) return;

	double tol = 1e-4;

	int nbDofsR = -1;

	// TODO :
	cout << _name << " : " << nbDofsR << " degree(s) of freedom in rotation\n";

	// Propagate to children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->nbDofs();
	}

}

