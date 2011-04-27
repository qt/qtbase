# use p4 where to get the solutions base dir.
# need to check for "..." since p4 behaves differently between versions
# SOLUTIONBASEDIR = $$system(p4 where //depot/addons/main/...)

# figure out the path on your harddrive
# SOLUTIONBASEDIR = $$member(SOLUTIONBASEDIR, 2)
SOLUTIONBASEDIR = $$(SOLUTIONBASEDIR)

# strip the trailing "..."
# SOLUTIONBASEDIR ~= s/\\.\\.\\.$//

# replace \ with /
# win32:SOLUTIONBASEDIR ~= s.\\\\./.g

isEmpty(SOLUTIONBASEDIR):DEFINES += QT_NO_SOLUTIONS

