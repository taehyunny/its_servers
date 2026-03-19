cmake_minimum_required(VERSION 3.10)
project(ItsServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 무식하지만 가장 확실한 방법: 모든 헤더 경로 직접 명시
# 이제 컴파일러는 이 모든 폴더를 이 잡듯이 뒤져서 헤더를 찾아냅니다.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/session
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
    ${CMAKE_CURRENT_SOURCE_DIR}/mariadb-connector-cpp/include
)

# 2. 소스 파일 목록 명시적 설정 (테스트 코드 완벽 차단)
file(GLOB SOURCES 
    "main.cpp"
    "src/database/*.cpp"
    "src/handler/*.cpp"
    "src/network/*.cpp"
    "src/service/*.cpp"
    "src/session/*.cpp"
    "src/thread/*.cpp"
    "src/util/*.cpp"
)

# 3. 정적 라이브러리 경로
set(MARIADB_LIB "/home/lms/Documents/its_server/server/mariadb-connector-cpp/build/libmariadbcpp-static.a")

# 4. 실행 파일 생성 및 링크
add_executable(ItsServer_App ${SOURCES})

target_link_libraries(ItsServer_App 
    ${MARIADB_LIB}
    mariadb
    pthread
    dl
    ssl
    crypto
    z
)