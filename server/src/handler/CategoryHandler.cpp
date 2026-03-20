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
        MainHomeResDTO res;
        res.status = 200;

        // 🚀 이제 컴파일러가 아주 평온하게 에러 없이 잘 받아들입니다.
        res.categories = CategoryDAO::getInstance().getAllCategories();
        res.topStores = StoreDAO::getInstance().getTopStoresByCategory();

        // 클라이언트에게 한 방에 전송!
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CATEGORY), res);

        std::cout << "[CategoryHandler] 메인 화면 데이터(카테고리 " << res.categories.size()
                  << "개, 1등 매장 " << res.topStores.size() << "개) 한방에 전송 완료!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CategoryHandler] 메인 화면 전송 에러: " << e.what() << std::endl;
    }
}