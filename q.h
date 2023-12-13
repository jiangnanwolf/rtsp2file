#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std::chrono_literals;

template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue(size_t max_size = std::numeric_limits<size_t>::max())
        : m_max_size(max_size), m_quit(false)
    {
    }

    template<typename... Args>
    void Emplace(Args&&... args){
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_quit; });
        if (!m_quit)
        {
            m_queue.emplace(std::forward<Args>(args)...);
            lock.unlock();
            m_cv_not_empty.notify_one();
        }
    }

    void Push(const T& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_quit; });
        if (!m_quit)
        {
            m_queue.push(value);
            lock.unlock();
            m_cv_not_empty.notify_one();
        }
    }

    void Push(T&& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_quit; });
        if (!m_quit)
        {
            m_queue.push(std::move(value));
            lock.unlock();
            m_cv_not_empty.notify_one();
        }
    }

    bool TryPop(T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        m_cv_not_full.notify_one();
        return true;
    }

    bool WaitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_not_empty.wait(lock, [this] { return !m_queue.empty() || m_quit; });
        if (!m_quit)
        {
            value = std::move(m_queue.front());
            m_queue.pop();
            lock.unlock();
            m_cv_not_full.notify_one();
            return true; // get value successfully
        }else{
            return false; // quit
        }
    }

    int WaitAndPopTimeout(T& value, int timeout_100ms = 1)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool result = m_cv_not_empty.wait_for(lock, timeout_100ms * 100ms, [this] { return !m_queue.empty() || m_quit; });
        if (result && !m_quit)
        {
            value = std::move(m_queue.front());
            m_queue.pop();
            lock.unlock();
            m_cv_not_full.notify_one();
            return 0; // get value successfully
        }
        else
        {
            // 1 for timeout, 2 for quit
            return m_quit ? 2 : 1; // quit or timeout
        }
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void Quit()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_quit = true;
        lock.unlock();
        m_cv_not_full.notify_all();
        m_cv_not_empty.notify_all();
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv_not_empty;
    std::condition_variable m_cv_not_full;
    size_t m_max_size;
    std::atomic<bool> m_quit;
};