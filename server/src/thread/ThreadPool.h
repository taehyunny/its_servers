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
    template <class F, class... Args>  // 가변 인자를 받아서 작업을 추가하는 함수
    void enqueue(F &&f, Args &&...args)// 작업을 스레드 풀에 추가하는 함수
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex); // 작업 큐에 접근하기 위한 락 (동기화)
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));  //  작업 큐에 새로운 작업 추가 (std::bind로 함수와 인자를 묶어서 저장)
        }
        condition.notify_one(); // 자고 있는 스레드 중 하나를 깨워서 새 작업이 들어왔음을 알림
    }

private:
    std::vector<std::thread> workers;  // 실제 작업을 수행하는 스레드들
    std::queue<std::function<void()>> tasks;  // 작업 큐: 실행할 작업들이 저장되는 곳

    std::mutex queue_mutex;   // 작업 큐에 접근하기 위한 뮤텍스 (동기화)
    std::condition_variable condition;  // 작업이 들어오거나 스레드 풀이 멈췄을 때 스레드를 깨우기 위한 조건 변수
    bool stop;  // 스레드 풀이 멈췄는지 여부를 나타내는 플래그 (true이면 스레드들이 작업을 마치고 종료하도록 신호)
};