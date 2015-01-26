
MIDL        = midl
MIDL_OPTS   = /dlldata NUL /proxy NUL /notlb /out ..\generated

first: all

all: x86 amd64

x86:
	-md ..\generated\x86
	$(MIDL) $(MIDL_OPTS)\x86 /env win32 ia2_api_all.idl


amd64:
	-md ..\generated\amd64
	$(MIDL) $(MIDL_OPTS)\amd64 /env amd64 ia2_api_all.idl

