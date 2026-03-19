#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool
{
public:
    // 생성자와 소멸자는 cpp 파일로 분리 (선언만 남김)
    ThreadPool(size_t threads);
    ~ThreadPool();

    // 템플릿 함수는 반드시 헤더 파일에 구현부가 있어야 함!
    template <class F, class... Args>
    void enqueue(F &&f, Args &&...args)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};