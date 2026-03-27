#pragma once
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

// 채팅방의 최소 단위 (논리적 방)
struct ChatRoom
{
    int roomId;
    std::string customerId;
    std::string adminId;
    uint64_t orderId;
    // 필요한 경우 메시지 로그를 잠시 담아둘 수도 있습니다.
};

class ChatRoomManager
{
public:
    static ChatRoomManager &getInstance()
    {
        static ChatRoomManager instance;
        return instance;
    }

    // 🚀 관리자가 수락했을 때: 새로운 1:1 통로 개설
    int createRoom(const std::string &customerId, const std::string &adminId, uint64_t orderId);

    // 🚀 메시지 전달 시: 상대방이 누구인지 찾기용
    std::shared_ptr<ChatRoom> getRoom(int roomId);

    // 🚀 모든 방 정보 가져오기 (메시지 라우팅용)
    std::vector<std::shared_ptr<ChatRoom>> getAllRooms();

    // 🚀 채팅 종료 시: 메모리 회수 (태현님이 강조하신 그 부분!)
    void removeRoom(int roomId);

private:
    ChatRoomManager() = default;

    std::unordered_map<int, std::shared_ptr<ChatRoom>> rooms;
    std::mutex roomMutex; // 16명의 워커가 싸우지 않게 방어!
    int nextRoomId = 1;
};