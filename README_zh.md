# trajToCluster

👉 [English](README.md)

trajToCluster：一个用于 [HYSPLIT](https://www.ready.noaa.gov/HYSPLIT.php) 的聚类分析工具。本软件增强了 HYSPLIT 的聚类分析功能，降低了内存占用，并支持多核并行计算。

|                                     ![speed comparison](speedComparison.svg)                                     |
|:----------------------------------------------------------------------------------------------------------------:|
| *HYSPLIT、trajToCluster（单核）和 trajToCluster（多核）进行聚类分析的运行时间对比（单位：s）。* |

## 依赖
本项目使用了 C++20 的部分特性，请确保编译器支持 C++20。代码依赖 `netCDF-cxx` 库、`TBB` 并行算法库以及 `cxxopts` 命令行参数解析库。用户可以通过 vcpkg（Windows）、apt（Ubuntu）或 Homebrew（macOS）等包管理器安装这些依赖。例如，使用 `vcpkg`：
```shell
vcpkg.exe install netcdf-cxx4:x64-windows
vcpkg.exe install tbb
vcpkg.exe install cxxopts
vcpkg.exe integrate install
```

## 安装
使用 `CMake` 构建本项目。

Windows：
```shell
mkdir build
cmake.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg root directory>\scripts\buildsystems\vcpkg.cmake -S <trajToCluster root directory> -B <trajToCluster root directory>\build
cmake.exe --build <trajToCluster root directory>\build --config Release --target trajToCluster
```
可执行文件位于 `build\Release`。

在其他操作系统上使用时，可能需要通过 `CMAKE_PREFIX_PATH` 等变量指定依赖库路径。如果你希望使用不同于 `find_package` 的依赖查找方式，也可以修改 `CMakeLists.txt`。

## 使用
请运行 `trajToCluster -h` 查看简要文档。
