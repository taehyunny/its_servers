cmake_minimum_required(VERSION 3.10)
project(ItsServer)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 소스 파일 및 헤더 경로 설정
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dto            # AllDTOs.h 등이 여기 있다면
    ${CMAKE_CURRENT_SOURCE_DIR}/src/session    # ClientSession.h, SessionManager.h 등이 있다면
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handler    # Global_protocol.h 등이 있다면
    ${CMAKE_CURRENT_SOURCE_DIR}/mariadb-connector-cpp/include
)

# 소스 파일 목록 (파일이 추가될 때마다 여기에 이름을 적어주세요)
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES "main.cpp")

# 2. 정적 라이브러리 파일 경로 정의 
# (mariadb-connector-cpp/lib 폴더 안에 해당 파일이 있는지 꼭 확인하세요!)
set(MARIADB_LIB "/home/lms/Documents/its_server/server/mariadb-connector-cpp/build/libmariadbcpp-static.a")

# 3. 실행 파일 생성
# 2. 실행 파일 생성
add_executable(ItsServer_App ${SOURCES})

# 3. 라이브러리 링크 (순서가 중요합니다!)
target_link_libraries(ItsServer_App 
    ${MARIADB_LIB}
    pthread
    dl
    ssl
    crypto
)
# 5. 빌드 후 전처리 (선택 사항)
# 빌드된 실행 파일에서 불필요한 심볼을 제거해 용량을 줄입니다.
add_custom_command(TARGET ItsServer_App POST_BUILD
    COMMAND ${CMAKE_STRIP} ItsServer_App
    COMMENT "Stripping binary to reduce size..."
)