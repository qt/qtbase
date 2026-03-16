# 
# BVH Importer script for Maya.
# 
# Importer for .bvh files (BioVision Hierachy files).
# BVH is a common ascii motion capture data format containing skeletal and motion data.
# 
# <license>
# BVH Importer script for Maya.
# Copyright (C) 2012  Jeroen Hoolmans
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# </license>

__author__ 		= "Jeroen Hoolmans"
__copyright__ 	= "Copyright 2012, Jeroen Hoolmans"
__credits__ 	= ["Jeroen Hoolmans"]
__license__ 	= "GPL"
__version__ 	= "1.0.1"
__maintainer__ 	= "Jeroen Hoolmans"
__email__ 		= "jhoolmans@gmail.com"
__status__ 		= "Production"

import pymel.core as pm
import maya.cmds as mc
import os

# This maps the BVH naming convention to Maya
translationDict = {
	"Xposition" : "translateX",
	"Yposition" : "translateY",
	"Zposition" : "translateZ",
	"Xrotation" : "rotateX",
	"Yrotation" : "rotateY",
	"Zrotation" : "rotateZ"
}


class TinyDAG(object):
	#
	# Small helper class to keep track of parents
	#
	
	def __init__(self, obj, pObj = None):
		self.obj = obj
		self.pObj = pObj
		
	def __str__(self):
		# returns object name
		return str(self.obj)
		
	def _fullPath(self):
		# returns full object path 
		if self.pObj is not None:
			return "%s|%s" % (self.pObj._fullPath(), self.__str__())
		return str(self.obj)
		
class BVHImporterDialog(object):
	#
	# Dialog class..
	#
	
	def __init__(self, debug=False):
		# Don't use debug when importing more than 10 frames.. Otherwise it gets messy
		self._name = "bvhImportDialog"
		self._title = "BVH Importer %s" % __version__
		
		# UI related
		self._textfield = ""
		self._scaleField = ""
		self._frameField = ""
		self._rotationOrder = ""
		self._reload = ""
		
		# Other
		self._rootNode = None # Used for targeting
		self._debug = debug
		
		# BVH specific stuff
		self._filename = ""
		self._channels = []
		
		self.setup_ui()
	
	def setup_ui(self):
		# Creates the great dialog
		win = self._name
		if mc.window(win, ex=True):
			mc.deleteUI(win)
		
		# Non sizeable dialog
		win = mc.window(self._name, title=self._title, w=200, rtf=True, sizeable=False)
		
		mc.columnLayout(adj=1, rs=5)
		mc.separator()
		mc.text("Options")
		mc.separator()
		
		mc.rowColumnLayout( numberOfColumns=2, 
			columnWidth=[(1, 80), (2, 150)], 
			cal=[(1, "right"), (2, "center")],
			cs=[(1,5), (2,5)],
			rs=[(1,5), (2,5)])
		
		mc.text("Rig scale")
		self._scaleField = mc.floatField(minValue=0.01, maxValue=2, value=1)
		mc.text("Frame offset")
		self._frameField = mc.intField(minValue=0)
		mc.text("Rotation Order")
		self._rotationOrder = mc.optionMenu()
		mc.menuItem( label='XYZ' )
		mc.menuItem( label='YZX' )
		mc.menuItem( label='ZXY' )
		mc.menuItem( label='XZY' )
		mc.menuItem( label='YXZ' )
		mc.menuItem( label='ZYX' )
		
		mc.setParent("..")
		mc.separator()
		
		# Targeting UI
		mc.text("Skeleton Targeting")
		mc.text("(Select the hips)")
		mc.separator()
		
		mc.rowColumnLayout( numberOfColumns=2, 
		columnWidth=[(1, 150), (2, 80)],
			cs=[(1,5), (2,5)],
			rs=[(1,5), (2,5)])
		
		self._textfield = mc.textField(editable=False)
		mc.button("Select/Clear", c=self._on_select_root)
		
		mc.setParent("..")
		mc.separator()
		mc.button("Import..", c=self._on_select_file)
		self._reload = mc.button("Reload", enable=False, c=self._read_bvh)
		
		# Sorry :)
		mc.text("Created by Jeroen Hoolmans")
		
		mc.window(win, e=True, rtf=True, sizeable=False)
		mc.showWindow(win)
		
	def _on_select_file(self, e):
		# Without All Files it didn't work for some reason..
		filter = "All Files (*.*);;Motion Capture (*.bvh)"
		dialog = mc.fileDialog2(fileFilter=filter, dialogStyle=1, fm=1)
		
		if dialog is None:
			return
		if not len(dialog):
			return
		
		self._filename = dialog[0]
		
		mc.button(self._reload, e=True, enable=True)
		
		# Action!
		self._read_bvh()
		
	def _read_bvh(self, e=False):
		# Safe close is needed for End Site part to keep from setting new parent.
		safeClose = False
		# Once motion is active, animate.
		motion = False
		# Clear channels before appending
		self._channels = []
		
		# Scale the entire rig and animation
		rigScale = mc.floatField(self._scaleField, q=True, value=True)
		frame = mc.intField(self._frameField, q=True, value=True)
		rotOrder = mc.optionMenu(self._rotationOrder, q=True, select=True) - 1
		
		with open(self._filename) as f:
			# Check to see if the file is valid (sort of)
			if not f.next().startswith("HIERARCHY"):
				mc.error("No valid .bvh file selected.")
				return False
			
			if self._rootNode is None:
				# Create a group for the rig, easier to scale. (Freeze transform when ungrouping please..)
				mocapName = os.path.basename(self._filename)
				grp = pm.group(em=True,name="_mocap_%s_grp" % mocapName)
				grp.scale.set(rigScale, rigScale, rigScale) 
				
				# The group is now the 'root'
				myParent = TinyDAG(str(grp), None)
			else:
				myParent = TinyDAG(str(self._rootNode), None)
				self._clear_animation()
			
			for line in f:
				line = line.replace("	"," ") # force spaces
				if not motion:
					# root joint
					if line.startswith("ROOT"):
						# Set the Hip joint as root
						if self._rootNode:
							myParent = TinyDAG(str(self._rootNode), None)
						else:
							myParent = TinyDAG(line[5:].rstrip(), myParent)
					
					if "JOINT" in line:
						jnt = line.split(" ")
						# Create the joint
						myParent = TinyDAG(jnt[-1].rstrip(), myParent)
	
					if "End Site" in line:
						# Finish up a hierarchy and ignore a closing bracket
						safeClose = True
	
					if "}" in line:
						# Ignore when safeClose is on
						if safeClose:
							safeClose = False
							continue
						
						# Go up one level
						if myParent is not None:
							myParent = myParent.pObj
							if myParent is not None:
								mc.select(myParent._fullPath())
							
					if "CHANNELS" in line:
						chan = line.strip().split(" ")
						if self._debug:
							print chan
						
						# Append the channels that are animated
						for i in range(int(chan[1]) ):
							self._channels.append("%s.%s" % (myParent._fullPath(), translationDict[chan[2 + i]] ) )
						
					if "OFFSET" in line:
						offset = line.strip().split(" ")
						if self._debug:
							print offset
						jntName = str(myParent)
						
						# When End Site is reached, name it "_tip"
						if safeClose:
							jntName += "_tip"
						
						# skip if exists
						if mc.objExists(myParent._fullPath()):
							jnt = pm.PyNode(myParent._fullPath())
							jnt.rotateOrder.set(rotOrder)
							jnt.translate.set([float(offset[1]), float(offset[2]), float(offset[3])])
							continue
						
						# Build the joint and set its properties
						jnt = pm.joint(name=jntName, p=(0,0,0))
						jnt.translate.set([float(offset[1]), float(offset[2]), float(offset[3])])
						jnt.rotateOrder.set(rotOrder)
					
					if "MOTION" in line:
						# Animate!
						motion = True
					
					if self._debug:
						if myParent is not None:
							print "parent: %s" % myParent._fullPath()
							
				else:
					# We don't really need to use Framecount and time(since Python handles file reads nicely)
					if "Frame" not in line:
						data = line.split(" ")
						if len(data) > 0:
							if data[0] == "": data.pop(0)
						
						if self._debug:
							print "Animating.."
							print "Data size: %d" % len(data)
							print "Channels size: %d" % len(self._channels)
						# Set the values to channels
						for x in range(0, len(data) - 1 ):
							if self._debug:
								print "Set Attribute: %s %f" % (self._channels[x], float(data[x]))
							mc.setKeyframe(self._channels[x], time=frame, value=float(data[x]))
						
						frame = frame + 1
	
	def _clear_animation(self):
		# select root joint
		pm.select(str(self._rootNode), hi=True)
		nodes = pm.ls(sl=True)
		
		trans_attrs = ["translateX", "translateY", "translateZ"]
		rot_attrs = ["rotateX", "rotateY", "rotateZ"]
		for node in nodes:
			for attr in trans_attrs:
				connections = node.attr(attr).inputs()
				pm.delete(connections)
			for attr in rot_attrs:
				connections = node.attr(attr).inputs()
				pm.delete(connections)
				node.attr(attr).set(0)
	
	def _on_select_root(self, e):
		# When targeting, set the root joint (Hips)
		selection = pm.ls(sl=True, type="joint")
		if len(selection) == 0:
			self._rootNode = None
			mc.textField(self._textfield, e=True, text="")
		else:
			self._rootNode = selection[0]
			mc.textField(self._textfield, e=True, text=str(self._rootNode))
		
if __name__ == "__main__":
	dialog = BVHImporterDialog()
