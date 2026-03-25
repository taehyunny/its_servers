#pragma once
#include <string>
#include "json.hpp"

// 주의: 헤더 파일에서의 using namespace는 피하는 것이 좋으므로 구조체 안에서 명시적으로 사용하거나 별도 처리합니다.
// 여기서는 실무적 편의를 위해 nlohmann::json을 직접 사용합니다.

struct BaseResponseDTO
{
    int status;          // 200: 성공, 400: 실패, 409: 충돌
    std::string message; // 클라이언트 UI 팝업용 메시지

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BaseResponseDTO, status, message)
};

struct CategoryItem
{
    int id;               // 1, 2, 3...
    std::string name;     // "한식", "중식", "일식"
    std::string iconPath; // "chinese.png" (클라이언트가 아이콘 그릴 때 씀)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CategoryItem, id, name, iconPath)
};

// 클라이언트에게 보낼 리스트(벡터) 패킷
struct CategoryListResDTO
{
    int status;                           // 200: 성공
    std::vector<CategoryItem> categories; // 🚀 여기에 카테고리들을 담아서 쏩니다!

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CategoryListResDTO, status, categories)
};

// 🙋‍♂️ 등급 변경 요청 (REQ_UPGRADE_NAME / REQ_DOWNGRADE_NAME)
struct ReqGradeUpdateDTO {
    std::string userId;
    std::string targetGrade; // 변경할 등급 (예: "VIP", "프리미엄", "일반")
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqGradeUpdateDTO, userId, targetGrade)
};

// 🙋‍♂️ 등급 변경 응답
struct ResGradeUpdateDTO {
    int status;          // 0: 성공, 1: 실패
    std::string message; 
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResGradeUpdateDTO, status, message)
};