#include "StoreDAO.h"
#include "DbManager.h" // ⚠️ 태현님 프로젝트의 실제 파일명에 맞게 대소문자 확인
#include <iostream>
#include <mariadb/conncpp.hpp>

std::vector<StoreDTO> StoreDAO::getAllStores() {
    std::vector<StoreDTO> storeList;

    try {
        // 1. 커넥션 풀에서 DB 연결 빌려오기
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());

        // 2. 전체 상점 목록 조회 쿼리 (실제 DB 컬럼명과 일치해야 합니다)
        std::string query = "SELECT store_id, store_name, category, status, delivery_fees, "
                            "cook_time, image_url, min_order_amount, rating, review_count, "
                            "delivery_time_range FROM STORE";
        
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

        // 3. 결과를 DTO 벡터에 차곡차곡 담기
        while (res->next()) {
            StoreDTO store;
            store.storeId = res->getInt("store_id");
            store.storeName = res->getString("store_name").c_str();
            store.category = res->getString("category").c_str();
            store.status = res->getInt("status");
            
            // 🚀 아담 그랜트의 디테일 포인트 1: JSON 파싱 처리
            // DB에는 문자열(VARCHAR/JSON)로 저장되어 있으므로, nlohmann 객체로 변환해야 합니다.
            std::string feesStr = res->getString("delivery_fees").c_str();
            if (!feesStr.empty()) {
                try {
                    store.deliveryFees = nlohmann::json::parse(feesStr);
                } catch (...) {
                    store.deliveryFees = nlohmann::json::object(); // 파싱 에러 시 빈 객체
                }
            } else {
                store.deliveryFees = nlohmann::json::object();
            }

            store.cookTime = res->getInt("cook_time");
            store.imageUrl = res->getString("image_url").c_str();
            store.minOrderAmount = res->getInt("min_order_amount");
            store.rating = res->getDouble("rating");
            store.reviewCount = res->getInt("review_count");
            store.deliveryTimeRange = res->getString("delivery_time_range").c_str();

            // ※ popularMenu(대표 메뉴)는 JOIN 쿼리가 복잡해지므로, 
            // 나중에 MenuManager가 캐싱을 완료한 뒤에 조립해 주는 것이 객체지향적으로 더 깔끔합니다.

            storeList.push_back(store);
        }
        std::cout << "[StoreDAO] DB에서 " << storeList.size() << "개의 상점 데이터를 로드했습니다." << std::endl;

    } catch (sql::SQLException& e) {
        std::cerr << "[FATAL] StoreDAO::getAllStores DB 에러: " << e.what() << std::endl;
    }

    return storeList;
}