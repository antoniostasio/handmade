@echo off

set vcvars_path="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
IF NOT EXIST %vcvars_path% (
	set vcvars_path="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)
call %vcvars_path%

set DIRCMD=/o

@REM set proj_root=%~dp0
@REM pushd "%proj_root%\.."
@REM set proj_root=%cd%
@REM popd

set path=t:\handmade\misc;%path%
