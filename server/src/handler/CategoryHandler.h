#pragma once
#include "ClientSession.h"
#include <nlohmann/json.hpp>

class CategoryHandler
{
public:
    // 클라이언트의 카테고리 목록 요청을 처리합니다.
    static void handleCategoryRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};