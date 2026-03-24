#include "ThreadPool.h"

// 생성자: 지정된 수만큼 Worker 스레드를 생성하여 대기 상태로 만듦
ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i) // 지정된 수만큼 Worker 스레드를 생성
    {                                    // 각 스레드는 무한 루프를 돌며 작업 큐에서 작업이 들어오기를 기다림
        workers.emplace_back([this]
                             {
            for(;;) {
                std::function<void()> task;// 작업 큐에서 실행할 작업을 꺼내기 위한 임시 변수
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);// 작업 큐에 접근하기 위한 락 (동기화)
                    // 작업이 들어오거나 중지 신호가 올 때까지 스레드 대기
                    this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });  // 대기 조건: stop이 true이거나 작업 큐에 작업이 있을 때
                    
                    // 스레드 풀이 멈췄고 남은 작업이 없다면 스레드 종료
                    if(this->stop && this->tasks.empty()) return;
                    
                    task = std::move(this->tasks.front());// 작업 큐에서 가장 앞에 있는 작업을 꺼냄
                    this->tasks.pop();  // 작업 큐에서 꺼낸 작업 제거
                }
                task(); // 실제 작업 실행
            } });
    }
}

// 소멸자: 모든 작업을 마치고 안전하게 스레드를 종료 (Graceful Shutdown)
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex); // 작업 큐에 접근하기 위한 락 (동기화)
        stop = true;
    }
    condition.notify_all(); // 자고 있는 모든 스레드를 깨움

    for (std::thread &worker : workers)
    {
        worker.join(); // 각 스레드가 하던 일을 마칠 때까지 기다림
    }
}