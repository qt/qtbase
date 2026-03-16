#ifndef _JOINT_H_
#define _JOINT_H_

#include <string>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <utility>

#include <QMatrix4x4>
#include <QQuaternion>

#include "super_parser.h"

class AnimCurve {
public :
	AnimCurve() {};
	~AnimCurve() {
		_values.clear();
	}
public :
	std::string name;					// name of dof
	std::vector<double> _values;		// different keyframes = animation curve
};


enum RotateOrder {roXYZ=0, roYZX, roZXY, roXZY, roYXZ, roZYX};

class Joint {
public :
	std::string _name;					// name of joint
	double _offX;						// initial offset in X
	double _offY;						// initial offset in Y
	double _offZ;						// initial offset in Z
	QMatrix4x4 *_transform;
	std::vector<AnimCurve> _dofs;		// keyframes : _animCurves[i][f] = i-th dof at frame f;
	double _curTx;						// current value of translation on X
	double _curTy;						// current value of translation on Y
	double _curTz;						// current value of translation on Z
	double _curRx;						// current value of rotation about X (deg)
	double _curRy;						// current value of rotation about Y (deg)
	double _curRz;						// current value of rotation about Z (deg)
	int _rorder;						// order of euler angles to reconstruct rotation
	std::vector<Joint*> _children;	// children of the current joint
	bool motion = false;
	Joint *parent = NULL;
	int index;
	double divider;

public :
	// Constructor :
	Joint() {};
	// Destructor :
	~Joint() {
		_dofs.clear();
		for (int i = 0; i < (int)_children.size(); i++){
			delete _children[i];
		}
		delete _transform;
		_children.clear();
	}

	// Create from data :
	static Joint* create(std::string name, double offX, double offY, double offZ, Joint* parent) {
		Joint* child = new Joint();
		child->divider = 1;
		if(parent == NULL){
			// a voir si on veut le scale
			child->divider = 1/1.0;
		}
		child->_name = name;
		child->_offX = offX;
		child->_offY = offY;
		child->_offZ = offZ;
		child->_transform = new QMatrix4x4();
		child->_transform->setToIdentity();
		QMatrix4x4 scale(child->divider, 0, 0, offX * child->divider, 0, child->divider, 0, offY * child->divider, 0, 0, child->divider, offZ * child->divider, 0, 0, 0, 1);
		*(child->_transform) = scale * *(child->_transform);
		child->_curTx = 0;
		child->_curTy = 0;
		child->_curTz = 0;
		child->_curRx = 0;
		child->_curRy = 0;
		child->_curRz = 0;
		if(parent != nullptr) {
			parent->_children.push_back(child);
		}
		child->parent = parent;
		return child;
	}

	// Load from file (.bvh) :	
	static std::pair<std::pair<Joint*, std::vector<Joint*>>, std::pair<int, double>> createFromFile(std::string fileName);

	void animate(int iframe=0);

	// Analysis of degrees of freedom :
	void nbDofs();
};


#endif