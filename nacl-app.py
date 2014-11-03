# build Qt applications for several Qt builds.
# usage: nacl-app app-source-dir qt-builds-dir

import argparse
from subprocess import Popen
from os import path
import os
import sys
import glob

parser = argparse.ArgumentParser()
parser.add_argument('input', nargs='*')
args = parser.parse_args()

appSourceDir = path.abspath(args.input[0])
qtBuildsDir = path.abspath(args.input[1])
appProFile = glob.glob(appSourceDir + '/*.pro')[0]
qtBuildNames = os.walk(qtBuildsDir).next()[1]

print ''
print 'app source dir: ' + appSourceDir
print 'app pro file  : '   + appProFile
print 'qt builds dir : ' + qtBuildsDir
print 'qt build names: ' + str(qtBuildNames)


#sys.exit(0)

for qtBuildName in qtBuildNames:
    print ''
    print 'now ' + qtBuildName

    # make a local build dir matching the Qt build dir name
    if not os.path.exists(qtBuildName):
        os.mkdir(qtBuildName)
    
        
    qtBuildDir = path.join(qtBuildsDir, qtBuildName)
    qmake = path.join(qtBuildDir, 'qtbase', 'bin', 'qmake')
    cmd = qmake + ' ' + appProFile + ' -r && make -k'
    print cmd
    Popen(cmd, shell=True, cwd=qtBuildName)
    
print ''    
    

