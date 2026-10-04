# GourmetAutoPatch.dll on alandtse's CommonLibSSE-NG; included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there)
ng_plugin(TARGET GourmetAutoPatch NAME GourmetAutoPatch VERSION 1.0.0 ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES src/main.cpp src/Config.cpp src/Classifier.cpp src/Conform.cpp src/Menu.cpp
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include"
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h" LIBS nlohmann_json::nlohmann_json)
