
## 环境要求

- Visual Studio 2022 (MSVC 19.x)
- Qt 6.8.3 (msvc2022_64)
- CMake 3.29+
- Ninja

## 构建方法

### 1. 配置

**重要：必须在 Visual Studio Developer Command Prompt (x64) 中执行，否则 MSVC 编译器无法检测。**

打开 "x64 Native Tools Command Prompt for VS 2022"，然后：

```bash
# Debug 配置
cmake -S . -B build/Desktop_Qt_6_8_3_MSVC2022_64bit-Debug -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64

# Release 配置
cmake -S . -B build/Desktop_Qt_6_8_3_MSVC2022_64bit-Release -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
```

> **常见问题：** 如果 Release 配置报 "No CMAKE_CXX_COMPILER could be found"，说明没有在 VS Developer Command Prompt 中运行，或者 `vcvars64.bat` 未正确加载。

### 2. 编译

```bash
# Debug 编译
cmake --build build/Desktop_Qt_6_8_3_MSVC2022_64bit-Debug

# Release 编译
cmake --build build/Desktop_Qt_6_8_3_MSVC2022_64bit-Release
```

### 3. 安装 (可选)

```bash
cmake --install build/Desktop_Qt_6_8_3_MSVC2022_64bit-Release --prefix "D:/MyIDE_SDK_R" --config Release
cmake --install build/Desktop_Qt_6_8_3_MSVC2022_64bit-Debug --prefix "D:/MyIDE_SDK_D" --config Debug
```
