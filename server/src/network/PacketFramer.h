#pragma once
#include <vector>
#include <string>
#include <memory> // 🚀 std::shared_ptr 사용을 위해 필수!
#include "ClientSession.h"
#include "Global_protocol.h"
#include "ThreadPool.h" // 🚀 ThreadPool 인식용

class PacketFramer
{
public:
    // 🚀 ThreadPool 참조(&) 매개변수 및 shared_ptr 적용
    static void onReceiveData(std::shared_ptr<ClientSession> session, const char *buffer, int length, ThreadPool &pool);

private:
    // 🚀 수정: 클래스 내부이므로 PacketFramer:: 생략, 끝에 세미콜론(;) 추가!
    // static 함수(onReceiveData)에서 호출하려면 이 녀석도 static이어야 합니다.
    static void processBuffer(std::shared_ptr<ClientSession> session, ThreadPool &pool);
};