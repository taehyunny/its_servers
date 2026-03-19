#include "ThreadPool.h"

// 생성자: 지정된 수만큼 Worker 스레드를 생성하여 대기 상태로 만듦
ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back([this]
                             {
            for(;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // 작업이 들어오거나 중지 신호가 올 때까지 스레드 대기
                    this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                    
                    // 스레드 풀이 멈췄고 남은 작업이 없다면 스레드 종료
                    if(this->stop && this->tasks.empty()) return;
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task(); // 실제 작업 실행
            } });
    }
}

// 소멸자: 모든 작업을 마치고 안전하게 스레드를 종료 (Graceful Shutdown)
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all(); // 자고 있는 모든 스레드를 깨움

    for (std::thread &worker : workers)
    {
        worker.join(); // 각 스레드가 하던 일을 마칠 때까지 기다림
    }
}