#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T> // T는 네트워크 이벤트, 클라이언트 요청 등 어떤 타입이든 될 수 있습니다.
class SafeQueue       // 스레드 안전한 큐입니다. 네트워크 스레드가 데이터를 넣고, 로직 스레드가 데이터를 꺼내는 구조에 최적화되어 있습니다.
{
public:
    SafeQueue() = default;
    ~SafeQueue() = default;

    // 🚀 데이터를 큐에 넣기 (Producer: 네트워크 스레드)
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex); // 자물쇠를 걸어서 큐에 안전하게 접근합니다.
        m_queue.push(std::move(value));            // 데이터를 큐에 넣습니다. std::move로 효율적으로 이동시킵니다.
        m_cond.notify_one();                       // 대기 중인 로직 스레드 하나를 깨움!
    }

    // 🚀 데이터를 꺼내기 (Consumer: 로직 스레드)
    // 데이터가 없으면 있을 때까지 잠시 대기합니다 (Blocking Pop)
    T wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex); // 자물쇠를 걸어서 큐에 안전하게 접근합니다.
        m_cond.wait(lock, [this]
                    { return !m_queue.empty(); }); // 큐가 비어있으면 대기합니다. 데이터가 들어오면 깨어납니다.

        T value = std::move(m_queue.front()); // 큐에서 데이터를 꺼냅니다. std::move로 효율적으로 이동시킵니다.
        m_queue.pop();
        return value;
    }

    // 🚀 데이터가 있는지 살짝 확인만 하기 (Non-blocking)
    std::optional<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex); // 자물쇠를 걸어서 큐에 안전하게 접근합니다.
        if (m_queue.empty())
            return std::nullopt; // 데이터가 없으면 nullopt를 반환합니다. 호출하는 쪽에서 std::optional을 체크해야 합니다.

        T value = std::move(m_queue.front()); // 큐에서 데이터를 꺼냅니다. std::move로 효율적으로 이동시킵니다.
        m_queue.pop();
        return value;
    }

    bool empty() const // 큐가 비어있는지 확인하는 함수입니다. 호출하는 쪽에서 큐의 상태를 체크할 때 유용합니다.
    {
        std::lock_guard<std::mutex> lock(m_mutex); // 자물쇠를 걸어서 큐에 안전하게 접근합니다.
        return m_queue.empty();                    // 큐가 비어있는지 확인합니다.
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};