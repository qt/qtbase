TEMPLATE = subdirs
SUBDIRS += compiler libGLESv2 libEGL
angle_d3d11: SUBDIRS += d3dcompiler
CONFIG += ordered
