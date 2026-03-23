#pragma once

#include <memory>
#include <string>

// 전방 선언
class ClientSession;

class StoreHandler
{
public:
    // 1. 매장 목록 조회 (전체 및 카테고리별)
    static void handleStoreListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 2. 매장 상세 정보 조회 (🚀 프론트엔드 요청 2002번)
    static void handleStoreDetailReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 3. 매장 정보 수정 (사장님 전용)
    static void handleStoreInfoUpdateReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 4. 영업 상태 변경 (사장님 전용)
    static void handleStoreStatusSet(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    
};