#include "CategoryHandler.h"
#include "AllDTOs.h"
#include <iostream>

void CategoryHandler::handleCategoryRequest(ClientSession *session, const std::string &jsonBody)
{
    std::cout << "[CategoryHandler] 클라이언트(fd: " << session->getFd() << ")가 카테고리 목록을 요청했습니다!" << std::endl;

    try
    {
        CategoryListResDTO res;
        res.status = 200;

        // 🚀 DB 연동 없이 하드코딩으로 빠르게 클라이언트에게 카테고리를 쏴줍니다!

        res.categories.push_back({1, "한식", "korean.png"});
        res.categories.push_back({2, "중식", "chinese.png"});
        res.categories.push_back({3, "돈까스/일식", "japanese.png"});
        res.categories.push_back({4, "양식", "western.png"});
        res.categories.push_back({5, "치킨", "chicken.png"});
        res.categories.push_back({6, "피자", "pizza.png"});
        res.categories.push_back({7, "햄버거", "burger.png"});
        res.categories.push_back({8, "족발/보쌈", "jokbal_bossam.png"});
        res.categories.push_back({9, "도시락", "lunchbox.png"});
        res.categories.push_back({10, "초밥/회", "sushi_sashimi.png"});

        // 1061번(RES_CATEGORY)으로 발사!
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CATEGORY), res);

        std::cout << "[CategoryHandler] 카테고리 " << res.categories.size() << "개 전송 완료!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CategoryHandler] 카테고리 전송 중 에러 발생: " << e.what() << std::endl;
    }
}