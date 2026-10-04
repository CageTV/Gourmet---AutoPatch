@echo off
rem Builds and runs the classifier test against work\corpus.tsv (run tools\make_corpus.py first)
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /std:c++20 /EHsc /O1 /utf-8 tools\classify_test.cpp plugin\src\Classifier.cpp /Fobuild\ /Febuild\classify_test.exe || exit /b 1
build\classify_test.exe work\corpus.tsv
