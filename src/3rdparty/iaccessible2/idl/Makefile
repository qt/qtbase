
MIDL        = midl
MIDL_OPTS   = /out ..\generated

first: all

all: x86 amd64

MIDL_OPTS   = /out ..\generated\x86 /env win32

x86:
	-del ..\generated\x86\dlldata.c
	-md ..\generated\x86
	$(MIDL) $(MIDL_OPTS) ia2_api_all.idl

MIDL_OPTS   = /out ..\generated\amd64 /env amd64

amd64:
	-del ..\generated\amd64\dlldata.c
	-md ..\generated\amd64
	$(MIDL) $(MIDL_OPTS) ia2_api_all.idl
