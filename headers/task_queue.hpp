/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef task_queue_hpp__
#define task_queue_hpp__

/******************************************************************************/

// stdc++
#include <deque>
#include <string>
#include <thread>

// application
#include "switches.hpp"

/******************************************************************************/

struct task_queue_t {
    typedef std::function<void ()>    task_t;
    typedef std::mutex                mutex_t;
    typedef std::unique_lock<mutex_t> lock_t;
    typedef std::deque<task_t>        task_deque_t;

    task_queue_t(std::size_t pool_size = std::thread::hardware_concurrency()) {
        for (std::size_t i(0); i < pool_size; ++i) {
            pool_m.emplace_back(std::bind(&task_queue_t::worker, this));
        }
    }

    ~task_queue_t() {
        lock_t lock{mutex_m};

        signal_done();

        lock.unlock();

        for (auto& thread : pool_m) {
            thread.join();
        }
    }

    template <typename F>
    void push(F&& function, priority_t priority = priority_t::normal) {
        lock_t lock{mutex_m};

        deque_m.emplace_back(std::forward<F>(function));

        condition_m.notify_one();
    }

    void signal_done() {
        if (done_m.exchange(true))
            return;

        condition_m.notify_all();
    }

private:
    task_queue_t(const task_queue_t&) = delete;
    task_queue_t(task_queue_t&&) = delete;
    task_queue_t& operator=(const task_queue_t&) = delete;
    task_queue_t& operator=(task_queue_t&&) = delete;

    void worker() {
        task_t task;

        while (true) try {
#if qMac
            pthread_setname_np("wait : worker");
#endif
            lock_t lock{mutex_m};

            condition_m.wait(lock, [=](){ return done_m || !empty_unsafe(); });

            if (done_m)
                return;

            if (try_pop_unsafe(task)) {
                lock.unlock();
#if qMac
                pthread_setname_np("RUNN : worker");
#endif
                task();
            }
        } catch (...) {
            // Drop it on the floor. Not ideal, but really there's nowhere
            // for them to go right now.
        }
    }

    bool empty_unsafe() const {
        return deque_m.empty();
    }

    bool try_pop_unsafe(task_t& task) {
        if (deque_m.empty())
            return false;

        task = deque_m.front();

        deque_m.pop_front();

        return true;
    }

    task_deque_t             deque_m;
    std::vector<std::thread> pool_m;
    std::condition_variable  condition_m;
    mutex_t                  mutex_m;
    std::atomic<bool>        done_m{false};
};

/******************************************************************************/

#endif // task_queue_hpp__

/******************************************************************************/
