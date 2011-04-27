# Test to see if include(), by default, succeeds when the specific file 
#   to include exists
include(existing_file.pri)

# Test to see if by specifying full set of parameters to include()
#  succeeds when the specified filed to include exists 
include(existing_file.pri, "", false)
