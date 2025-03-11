cmake_minimum_required(VERSION 3.8)  # 不要修改
project(CodeCraft)                   # 不要修改

set(CMAKE_CXX_STANDARD      17)      # 不要修改
set(CMAKE_C_STANDARD        11)      # 不要修改

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/) # 不要修改
option(BUILD_SHARED_LIBS    ""  OFF) # 不要修改

# 如果未指定构建类型，则默认使用 Release 模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("./include")
aux_source_directory(./                     cur_src) # 不要修改

# 添加最高级别的编译优化和多线程支持
# -O3：最高级别优化
# -march=native：针对本机架构优化
# -pthread：开启多线程支持
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -pthread")
set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -march=native -pthread")

# 忽略unused-result警告，解决scanf返回值未被使用的警告
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")

add_executable(code_craft                   ${cur_src}) # ！！！不要修改 code_craft 名称，直接影响结果；可以根据语法在 ${cur_src} 后面追加

# 链接pthread
target_link_libraries(code_craft pthread rt m)
