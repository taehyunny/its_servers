#pragma once
#include <string>
#include <memory>
#include "ClientSession.h"

class SalesHandler
{
public:
    // 매출 통계 조회 (REQ_SALES_STAT = 3090)
    static void handleSalesStat(std::shared_ptr<ClientSession> session,
                                const std::string &jsonBody);
};
