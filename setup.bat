@echo off

pushd %CD%
call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvars32.bat"
popd
