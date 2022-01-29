//#include <utility>
//#include <deque>
//#include <future>
//
//#include "teams.h"
//
//ContestResult TeamNewThreads::runContestImpl(ContestInput const & contestInput)
//{
//    ContestResult r;
//    r.resize(contestInput.size());
//    uint64_t idx = 0;
//
//    std::vector<std::promise<uint32_t>> promises(r.size());
//    for (auto & promise : promises)
//    {
//        std::promise<uint32_t> p;
//        promise = std::move(p);
//    }
//
//    std::vector<std::future<uint32_t>> futures(r.size());
//    for (auto i = 0; i < futures.size(); ++i)
//        futures[i] = promises[i].get_future();
//
//    std::vector<std::thread> threads(r.size());
//    for (auto i = 0; i < contestInput.size(); ++i)
//    {
//        if (getCreatedThreads() >= getSize())
//        {
//            r[idx] = futures[idx].get();
//            threads[idx].join();
//            idx++;
//        }
//        std::thread t = createThread(
//                [&, n = contestInput[i], &promise = promises[i]] {
//                    if (this->getSharedResults())
//                        promise.set_value(calcCollatzShare(n, std::shared_ptr<SharedResults>(this->getSharedResults())));
//                    else
//                        promise.set_value(calcCollatz(n));
//                });
//        threads[i] = std::move(t);
//    }
//
//    while (idx < r.size())
//    {
//        r[idx] = futures[idx].get();
//        threads[idx].join();
//        idx++;
//    }
//
//    return r;
//}
//
//ContestResult TeamConstThreads::runContestImpl(ContestInput const & contestInput)
//{
//    ContestResult r;
//    r.resize(contestInput.size());
//    auto thread_cnt = getSize();
//
//    std::vector<std::promise<uint32_t>> promises(r.size());
//    for (auto & promise : promises)
//    {
//        std::promise<uint32_t> p;
//        promise = std::move(p);
//    }
//
//    std::vector<std::future<uint32_t>> futures(r.size());
//    for (auto i = 0; i < futures.size(); ++i)
//        futures[i] = promises[i].get_future();
//
//    std::vector<std::thread> threads;
//    for (auto i = 0; i < thread_cnt; ++i)
//    {
//        std::thread t = createThread([&]{
//            for (auto j = i; j < contestInput.size(); j += thread_cnt)
//            {
//                if (this->getSharedResults())
//                    promises[j].set_value(calcCollatzShare(contestInput[j], std::shared_ptr<SharedResults>(this->getSharedResults())));
//                else
//                    promises[j].set_value(calcCollatz(contestInput[j]));
//            }
//        });
//        threads.push_back(std::move(t));
//    }
//
//    for (auto i = 0; i < r.size(); ++i)
//        r[i] = futures[i].get();
//
//    for (auto & thread : threads)
//        thread.join();
//
//    return r;
//}
//
//ContestResult TeamPool::runContest(ContestInput const & contestInput)
//{
//    ContestResult r;
//    //TODO
//    return r;
//}
//
//ContestResult TeamNewProcesses::runContest(ContestInput const & contestInput)
//{
//    ContestResult r;
//    //TODO
//    return r;
//}
//
//ContestResult TeamConstProcesses::runContest(ContestInput const & contestInput)
//{
//    ContestResult r;
//    //TODO
//    return r;
//}
//
//ContestResult TeamAsync::runContest(ContestInput const & contestInput)
//{
//    ContestResult r;
//    //TODO
//    return r;
//}
