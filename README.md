# flvParser

## windows端构建工程
cmake -G "Visual Studio 16 2019" -A Win32 ./ -B .\cmake\build\

## mac端构建工程
### 生成xcode工程
cmake -DCMAKE_CXX_STANDARD=11 -GXcode  ./ -B ./cmake/build_mac

### 生成makefile
cmake -DCMAKE_CXX_STANDARD=11 -DCMAKE_BUILD_TYPE=Release ./ -B ./cmake/build_mac
