#include "ChatRoomManager.h"
#include <iostream>

// 🚀 새로운 1:1 채팅방 개설
int ChatRoomManager::createRoom(const std::string &customerId, const std::string &adminId, uint64_t orderId)
{
    std::lock_guard<std::mutex> lock(roomMutex); // 16명의 워커가 동시에 방을 만들지 못하게 잠금!

    int roomId = nextRoomId++;

    // 논리적 채팅방 객체 생성
    auto newRoom = std::make_shared<ChatRoom>();
    newRoom->roomId = roomId;
    newRoom->customerId = customerId;
    newRoom->adminId = adminId;
    newRoom->orderId = orderId;

    // 맵에 저장
    rooms[roomId] = newRoom;

    std::cout << "[ChatRoomManager] 🏠 방 생성 완료! ID: " << roomId
              << " (고객: " << customerId << " <-> 관리자: " << adminId << ")" << std::endl;

    return roomId;
}

// 🚀 특정 방 정보 가져오기 (메시지 라우팅용)
std::shared_ptr<ChatRoom> ChatRoomManager::getRoom(int roomId)
{
    std::lock_guard<std::mutex> lock(roomMutex);

    auto it = rooms.find(roomId);
    if (it != rooms.end())
    {
        return it->second;
    }

    return nullptr; // 방이 없으면 nullptr 반환
}
std::vector<std::shared_ptr<ChatRoom>> ChatRoomManager::getAllRooms()
{
    std::lock_guard<std::mutex> lock(roomMutex); // 읽을 때도 락을 걸어 동시성 문제 방지!
    std::vector<std::shared_ptr<ChatRoom>> roomList;

    // 맵(rooms)을 순회하면서 모든 방을 벡터에 담습니다.
    for (const auto &pair : rooms)
    {
        roomList.push_back(pair.second);
    }

    return roomList;
}

// 🚀 채팅 종료 시 메모리 회수 (태현님이 말씀하신 가장 중요한 부분!)
void ChatRoomManager::removeRoom(int roomId)
{
    std::lock_guard<std::mutex> lock(roomMutex);

    if (rooms.erase(roomId))
    {
        std::cout << "[ChatRoomManager] ♻️ 방 " << roomId << " 삭제 및 메모리 회수 완료." << std::endl;
    }
    else
    {
        std::cout << "[ChatRoomManager] ⚠️ 삭제 실패: 방 " << roomId << "를 찾을 수 없습니다." << std::endl;
    }
}