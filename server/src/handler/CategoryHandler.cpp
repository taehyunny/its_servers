#include "CategoryHandler.h"
#include "CategoryDAO.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include <iostream>

void CategoryHandler::handleCategoryRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    std::cout << "[CategoryHandler] 클라이언트(fd: " << session->getFd() << ") 메인 화면 통합 데이터 요청!" << std::endl;

    try
    {
        // 🚀 1. 통합 바구니 준비
        MainHomeResDTO res;
        res.status = 200;

        // 🚀 2. DB에서 각각 긁어와서 바구니에 담기
        res.categories = CategoryDAO::getInstance().getAllCategories();
        res.topStores = StoreDAO::getInstance().getTopStoresByCategory();

        // 🚀 3. 클라이언트에게 단 한 번의 패킷(1061번)으로 묵직하게 발사!
        // (기존의 RES_CATEGORY = 1061 을 그대로 사용하면 됩니다)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CATEGORY), res);

        std::cout << "[CategoryHandler] 메인 화면 데이터(카테고리 " << res.categories.size()
                  << "개, 1등 매장 " << res.topStores.size() << "개) 한방에 전송 완료!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CategoryHandler] 메인 화면 전송 에러: " << e.what() << std::endl;
    }
}