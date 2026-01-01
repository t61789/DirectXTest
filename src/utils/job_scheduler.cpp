#include "job_scheduler.h"

#include <mutex>
#include <tracy/Tracy.hpp>

#include "common/utils.h"

namespace dt
{
    void Job::WaitForStart()
    {
        ZoneScopedC(TRACY_IDLE_COLOR);

        std::unique_lock lock(m_accessMutex);
        
        m_startingSignal.wait(lock, [this]
        {
            return m_exceptTaskNum != 0;
        });
    }

    void Job::WaitForStop()
    {
        ZoneScopedC(TRACY_IDLE_COLOR);

        assert(m_exceptTaskNum != 0 && "Job has not been scheduled");
        
        std::unique_lock lock(m_accessMutex);
        
        m_completeSignal.wait(lock, [this]
        {
            return m_completed.load();
        });
    }

    void Job::AppendNext(crsp<Job> next)
    {
        std::lock_guard lock(m_accessMutex);

        m_next.push_back(next);
    }

    bool Job::CompleteOnce()
    {
        std::lock_guard lock(m_accessMutex);
        
        m_completeTaskNum++;
        if (m_completeTaskNum == m_exceptTaskNum)
        {
            m_completed = true;
            return true;
        }
        return false;
    }
    
    
    JobScheduler::JobScheduler()
    {
        m_threadPool = std::make_unique<ThreadPool>(JOB_THREAD_COUNT);
    }

    JobScheduler::~JobScheduler()
    {
        while (!m_runningParallelTasks.empty())
        {
            sp<Job> job = nullptr;
            
            {
                std::lock_guard lock(m_schedulerMutex);
                if (m_runningParallelTasks.empty())
                {
                    break;
                }
                job = m_runningParallelTasks[0].second;
            }
            
            job->WaitForStop();
        }
        
        m_threadPool.reset();
    }

    void JobScheduler::Schedule(crsp<Job> job)
    {
        std::lock_guard lock0(m_schedulerMutex);
        std::lock_guard lock1(job->m_accessMutex);
        
        assert(!job->m_completed);
        assert(job->m_exceptTaskNum == 0);
        assert(!find(m_runningParallelTasks, job->m_taskGroupId));
        assert(!(job->m_taskFunc == nullptr && job->m_parallelTaskFunc == nullptr));
        assert(!(job->m_taskFunc != nullptr && job->m_parallelTaskFunc != nullptr));
        
        if (job->m_taskFunc != nullptr)
        {
            ScheduleCommonJob(job);
        }
        else if (job->m_parallelTaskFunc != nullptr)
        {
            ScheduleParallelJob(job);
        }
        
        insert(m_runningParallelTasks, job->m_taskGroupId, job);

        job->m_startingSignal.notify_all();
    }

    void JobScheduler::ScheduleCommonJob(crsp<Job> job)
    {
        job->m_exceptTaskNum = 1;
        job->m_completeTaskNum = 0;
        
        m_threadPool->Run([job, this]
        {
            (*job->m_taskFunc)();

            if (job->CompleteOnce())
            {
                this->JobComplete(job);
            }
        }, job->m_priority);
    }

    void JobScheduler::ScheduleParallelJob(crsp<Job> job)
    {
        assert(job->m_minBatchSize > 0);
        assert(job->m_taskElemCount != 0);

        auto batchSize = ceil_div(job->m_taskElemCount, JOB_THREAD_COUNT * 5);
        batchSize = (std::max)(batchSize, job->m_minBatchSize);
        assert(batchSize > 0);
        
        job->m_exceptTaskNum = 0;
        for (uint32_t start = 0; start < job->m_taskElemCount; start += batchSize)
        {
            job->m_exceptTaskNum++;
        }
        
        for (uint32_t start = 0; start < job->m_taskElemCount; start += batchSize)
        {
            auto end = (std::min)(start + batchSize, job->m_taskElemCount);
            m_threadPool->Run([start, end, job, this]
            {
                (*job->m_parallelTaskFunc)(start, end);

                if (job->CompleteOnce())
                {
                    this->JobComplete(job);
                }
            }, job->m_priority);
        }
    }
    
    void JobScheduler::JobComplete(crsp<Job> job)
    {
        for (auto& next : job->m_next)
        {
            Schedule(next);
        }
        
        {
            std::lock_guard lock(this->m_schedulerMutex);
            remove(this->m_runningParallelTasks, job->m_taskGroupId);
        }

        job->m_completeSignal.notify_all();
    }
}
