@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4127 -FC -Z7
set CommonLinkerFlags=-incremental:no -opt:ref glu32.lib opengl32.lib ..\src\lib\glew32.lib ..\src\lib\glfw3.lib ..\src\lib\glfw3dll.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl %CommonCompilerFlags% ..\src\main.cpp -Fmmain.map /link %CommonLinkerFlags%
popd
