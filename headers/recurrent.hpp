/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef recurrent_hpp__
#define recurrent_hpp__

/******************************************************************************/

// stdc++
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>

// application
#include "task_queue.hpp"

/******************************************************************************/

namespace recur {

/******************************************************************************/

typedef std::chrono::high_resolution_clock clock_t;
typedef std::function<void ()>             function_t;

struct token_t {
    explicit token_t(std::size_t id = 0) : id_m{id} { }

    std::size_t id_m;
};

inline bool operator==(const token_t& x, const token_t& y) {
    return x.id_m == y.id_m;
}

inline bool operator!=(const token_t& x, const token_t& y) {
    return !(x==y);
}

struct job_t {
    token_t           token_m;
    clock_t::duration interval_m;
    function_t        function_m;
};

struct engine_t {
    typedef std::mutex                           mutex_t;   
    typedef std::unique_lock<mutex_t>            lock_t;
    typedef std::map<clock_t::time_point, job_t> job_map_t;

    engine_t(task_queue_t& queue) : queue_m(queue) {
    }

    ~engine_t() {
        lock_t lock{mutex_m};

        terminate();
    }

    template <typename F>
    token_t insert(clock_t::duration interval, F&& function) {
        lock_t  lock{mutex_m};
        job_t   job{token_t{++id_m}, interval, std::forward<F>(function)};
        token_t result{job.token_m};

        schedule_unsafe(std::move(job));

        queue_check();

        return result;
    }

    void invoke(token_t token) {
        lock_t lock{mutex_m};

        for (auto iter(begin(jobs_m)), last(end(jobs_m)); iter != last; ++iter) {
            if (iter->second.token_m != token)
                continue;

            do_job(iter, lock);

            queue_check();

            break;
        }
    }

    void erase(token_t token) {
        lock_t lock{mutex_m};

        for (auto iter(begin(jobs_m)), last(end(jobs_m)); iter != last; ++iter) {
            if (iter->second.token_m != token)
                continue;

            jobs_m.erase(iter);

            queue_check();

            break;
        }
    }

    void terminate() {
        if (done_m.exchange(true))
            return;

        condition_m.notify_one();
    }

    void run() {
#if !defined(NDEBUG)
        pthread_setname_np("recurring");
#endif

        while (true) try {
            lock_t lock{mutex_m};

            condition_m.wait_until(lock, next_wakeup_unsafe());

            if (done_m) {
                return;
            }

            if (!running_m ||
                    jobs_m.empty() ||
                    next_wakeup_unsafe() > clock_t::now()) {
                continue;
            }

            do_job(jobs_m.begin(), lock);
        } catch (const std::exception& error) {
            std::cerr << "recur::engine_t error: " << error.what() << '\n';
        } catch (...) {
            std::cerr << "recur::engine_t error: unknown\n";
        }
    }

    bool done() const {
        return done_m;
    }

    bool running() const {
        return running_m;
    }

    void pause() {
        lock_t lock{mutex_m};

        if (!running_m.exchange(false))
            return;

        queue_check();
    }

    void resume() {
        lock_t lock{mutex_m};

        if (running_m.exchange(true))
            return;

        queue_check();
    }

private:
    engine_t(const engine_t&) = delete;
    engine_t(engine_t&&) = delete;
    engine_t& operator=(const engine_t&) = delete;
    engine_t& operator=(engine_t&&) = delete;

    void queue_check() {
        condition_m.notify_one();
    }

    void inner_do_job(job_t job) {
        try {
            job.function_m();
        } catch (const std::exception& error) {
            std::cerr << "Job error: " << error.what() << '\n';
        } catch (...) {
            std::cerr << "Job error: unknown\n";
        }

        lock_t lock{mutex_m};

        schedule_unsafe(std::move(job));

        queue_check();
    }

    void do_job(job_map_t::iterator job_iter, lock_t& lock) {
        job_t job(unschedule_unsafe(job_iter));

        lock.unlock();

        queue_m.push(std::bind(&engine_t::inner_do_job, std::ref(*this), std::move(job)));
    }

    void schedule_unsafe(job_t&& job) {
        clock_t::time_point next{clock_t::now() + job.interval_m};

        while (jobs_m.count(next))
            next += std::chrono::milliseconds(1);

        jobs_m.emplace(next, std::move(job));
    }

    job_t unschedule_unsafe(job_map_t::iterator job_iter) {
        job_t result(std::move(job_iter->second));

        jobs_m.erase(job_iter);

        return result;
    }

    clock_t::time_point next_wakeup_unsafe() {
        clock_t::time_point result{clock_t::time_point::max()};

        if (running_m && !jobs_m.empty())
            result = jobs_m.begin()->first;

        return result;
    }

    task_queue_t&            queue_m;
    std::atomic<std::size_t> id_m{0};
    job_map_t                jobs_m;
    mutex_t                  mutex_m;
    std::condition_variable  condition_m;
    std::atomic<bool>        done_m{false};
    std::atomic<bool>        running_m{true};
};

/******************************************************************************/

} // namespace recur

/******************************************************************************/

#endif // recurrent_hpp__

/******************************************************************************/
