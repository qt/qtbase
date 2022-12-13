#include "joint.h"
//#include <QtGui/QMatrix4x4>

using namespace std;

Joint* Joint::createFromFile(std::string fileName) {
	Joint* root = NULL;
	cout << "Loading from " << fileName << endl;

	ifstream inputfile(fileName.data());
	if(inputfile.good()) {

		bool isRoot = false;

		while(!inputfile.eof()) {
			string buf;	
			std::getline(inputfile, buf);
			double offX = 0;
			double offY = 0;
			double offZ = 0;
			string name;
			Joint *parent = NULL;
			Joint *currentJoint = NULL;

			buf.replace(0, string::npos, '	', ' ');
			if(buf.find("HIERARCHY") != string::npos) {
				cout<<"FOUND HIERARCHY"<<endl;
				continue;
			}

			if (buf.find("ROOT") != string::npos) {
				cout<<"FOUND ROOT"<<endl;
				name = buf.substr(5, string::npos);
				isRoot = true;
			}

			if (buf.find("JOINT") != string::npos) {
				cout<<"FOUND JOINT"<<endl;
				name = buf.substr(6, string::npos);
				isRoot = false;
			}

			if (buf.find("End") != string::npos) {
				cout<<"FOUND END"<<endl;
				name = buf.substr(4, string::npos);
			}
			
			if (buf.find("OFFSET") != string::npos) {
				cout<<"FOUND OFFSET"<<endl;
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
			}			

			if (buf.find("}") != string::npos) {
				cout<<"FOUND }"<<endl;
				currentJoint = currentJoint->parent;
				parent = currentJoint->parent;
			}

			size_t foundChannels = buf.find("CHANNELS");
			if (foundChannels != std::string::npos) {
				
			}

			if(isRoot){
				cout<<"CHANGING ROOT"<<endl;
				*root = *currentJoint;
			}
		}
		inputfile.close();
	} else {
		std::cerr << "Failed to load the file " << fileName.data() << std::endl;
		fflush(stdout);
	}

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

