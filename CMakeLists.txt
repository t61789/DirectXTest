cmake_minimum_required(VERSION 3.15)
project(DirectXTest LANGUAGES C CXX)

# 在 include(conanbuildinfo.cmake) 之前添加
list(APPEND CMAKE_PREFIX_PATH "build/Debug/generators")
include(E:/Programming/DirectXTest/build/generators/conan_toolchain.cmake)


# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SUPPRESS_REGENERATION TRUE)

# 编译器特定设置
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang 特定选项
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

file(GLOB_RECURSE SOURCES 
	src/*
)

include_directories(src)

add_executable(${PROJECT_NAME} 
	WIN32
	${SOURCES}
)

# 查找依赖包
find_package(nlohmann_json REQUIRED)
find_package(DirectX-Headers REQUIRED)
find_package(assimp REQUIRED)
find_package(Tracy REQUIRED)
find_package(Boost REQUIRED)
find_package(stb REQUIRED)

# 链接依赖库
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
		dxguid
		d3d12
		dxgi
		d3dcompiler
		windowscodecs
		nlohmann_json::nlohmann_json
		Microsoft::DirectX-Headers
		assimp::assimp
		Tracy::TracyClient
		boost::boost
		stb::stb
)

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)