@echo off

REM -MTd for debug build
set commonFlagsCompiler=-MTd -nologo -Gm- -GR- -fp:fast -EHa -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4130 -wd4996 -FC -Z7
set commonFlagsLinker= -incremental:no -opt:ref -subsystem:console

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl %commonFlagsCompiler% ..\code\ttr_setupasst.cpp /link -incremental:no -opt:ref wininet.lib
popd
