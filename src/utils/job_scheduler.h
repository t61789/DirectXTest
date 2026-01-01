#pragma once
#include <functional>
#include <mutex>
#include <tracy/Tracy.hpp>

#include "common/const.h"
#include "utils/thread_pool.h"

namespace dt
{
    using TaskFunc = std::function<void()>;
    using ParallelTaskFunc = std::function<void(uint32_t, uint32_t)>;

    class Job
    {
    public:
        Job() = default;

        void WaitForStart();
        void WaitForStop();
        void AppendNext(crsp<Job> next);

        bool IsComplete() const { return m_completed.load(); }
        void SetMinBatchSize(const uint32_t minBatchSize) { m_minBatchSize = minBatchSize; }
        void SetPriority(const int32_t priority) { m_priority = priority; }
        
        template <typename TaskFunc>
        static sp<Job> CreateCommon(TaskFunc&& f);
        template <typename TaskFunc>
        static sp<Job> CreateParallel(uint32_t taskElemCount, TaskFunc&& f);

    private:
        int32_t m_priority = 0;
        uint32_t m_taskElemCount = 0;
        uint32_t m_minBatchSize = 32;
        size_t m_taskGroupId = 0;
        
        std::optional<TaskFunc> m_taskFunc = std::nullopt;
        std::optional<ParallelTaskFunc> m_parallelTaskFunc = std::nullopt;
        
        uint32_t m_exceptTaskNum = 0;
        uint32_t m_completeTaskNum = 0;
        std::atomic_bool m_completed = false;

        vecsp<Job> m_next;
        
        std::mutex m_accessMutex;
        std::condition_variable m_startingSignal;
        std::condition_variable m_completeSignal;
        
        bool CompleteOnce();

        friend class JobScheduler;
    };
    
    class JobScheduler
    {
    public:
        JobScheduler();
        ~JobScheduler();
        JobScheduler(const JobScheduler& other) = delete;
        JobScheduler(JobScheduler&& other) noexcept = delete;
        JobScheduler& operator=(const JobScheduler& other) = delete;
        JobScheduler& operator=(JobScheduler&& other) noexcept = delete;

        void Schedule(crsp<Job> job);

    private:

        up<ThreadPool> m_threadPool;
        vec<std::pair<size_t, sp<Job>>> m_runningParallelTasks;
        std::mutex m_schedulerMutex;

        void ScheduleCommonJob(crsp<Job> job);
        void ScheduleParallelJob(crsp<Job> job);
        void JobComplete(crsp<Job> job);
    };

    template <typename CommonTaskFunc>
    sp<Job> Job::CreateCommon(CommonTaskFunc&& f)
    {
        auto job = msp<Job>();
        job->m_taskGroupId = std::hash<size_t>{}(reinterpret_cast<uintptr_t>(job.get()));
        job->m_taskFunc = std::forward<CommonTaskFunc>(f);

        return job;
    }

    template <typename ParallelTaskFunc>
    sp<Job> Job::CreateParallel(const uint32_t taskElemCount, ParallelTaskFunc&& f)
    {
        auto job = msp<Job>();
        job->m_taskElemCount = taskElemCount;
        job->m_taskGroupId = std::hash<size_t>{}(reinterpret_cast<uintptr_t>(job.get()));
        job->m_parallelTaskFunc = std::forward<ParallelTaskFunc>(f);

        return job;
    }
}
