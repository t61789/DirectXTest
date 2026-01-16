cmake_minimum_required(VERSION 3.15)
project(DirectXTest LANGUAGES C CXX)

# 在 include(conanbuildinfo.cmake) 之前添加
list(APPEND CMAKE_PREFIX_PATH "build/generators")
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

option(USE_ASAN "Enable Address Sanitizer" OFF)

if(USE_ASAN)
    if(MSVC)
        # 开启 ASan 标志
		add_compile_options(/fsanitize=address)
		
		# 必须开启调试符号，否则 ASan 报告没有行号
		add_compile_options(/Zi) 
		
		# 链接器也需要开启 ASan
		add_link_options(/fsanitize=address)
		
		# 禁用增量链接（ASan 不支持）
		string(REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    else()
        # GCC/Clang
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
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
find_package(imgui REQUIRED)

get_target_property(IMGUI_INCLUDE_DIRS imgui::imgui INTERFACE_INCLUDE_DIRECTORIES)

foreach(_P ${IMGUI_INCLUDE_DIRS})
    # 如果路径里包含盘符（C:）或者斜杠，说明这部分是路径
    if(_P MATCHES ":/" OR _P MATCHES "/")
        # 清洗掉左边的表达式：把直至第一个冒号（:）之前的内容删掉
        # 比如把 $<$<CONFIG:Debug>:C:/path 变成 C:/path
        string(REGEX REPLACE "^.*Debug\>:" "" _CLEAN "${_P}")
        
        # 清洗掉右边的括号：把末尾的 > 删掉
        string(REPLACE ">" "" _CLEAN "${_CLEAN}")
        
        # 得到纯净路径
        set(IMGUI_INCLUDE_DIR "${_CLEAN}")
        break()
    endif()
endforeach()

get_filename_component(IMGUI_PACKAGE_ROOT "${IMGUI_INCLUDE_DIR}" DIRECTORY)
get_filename_component(IMGUI_PACKAGE_ROOT "${IMGUI_PACKAGE_ROOT}" DIRECTORY)
set(IMGUI_BACKEND_DIR "${IMGUI_PACKAGE_ROOT}/b/src/backends")

target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_BACKEND_DIR})
target_sources(${PROJECT_NAME} PRIVATE 
    "${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp"
    "${IMGUI_BACKEND_DIR}/imgui_impl_dx12.cpp"
)

# 链接依赖库
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
		dxguid
		d3d12
		dxgi
		d3dcompiler
		dxcompiler
		windowscodecs
		nlohmann_json::nlohmann_json
		Microsoft::DirectX-Headers
		assimp::assimp
		Tracy::TracyClient
		boost::boost
		stb::stb
		imgui::imgui
)

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)