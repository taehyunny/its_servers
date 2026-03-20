#pragma once
#include <vector>
#include <string>
#include <memory>
#include "AllDTOs.h" 

class CategoryDAO
{
public:
    static CategoryDAO& getInstance()
    {
        static CategoryDAO instance;
        return instance;
    }

    // 🚀 CategoryInfo 대신 CategoryItem 사용!
    std::vector<CategoryItem> getAllCategories();

private:
    CategoryDAO() = default;
    ~CategoryDAO() = default;
    CategoryDAO(const CategoryDAO&) = delete;
    CategoryDAO& operator=(const CategoryDAO&) = delete;
};