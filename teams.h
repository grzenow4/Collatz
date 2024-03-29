#ifndef TEAMS_HPP
#define TEAMS_HPP

#include <thread>
#include <assert.h>

#include "lib/rtimers/cxx11.h"
#include "lib/pool/cxxpool.h"

#include "contest.h"
#include "collatz.h"
#include "sharedresults.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

class Team
{
public:
    Team(uint32_t sizeArg, bool shareResults): size(sizeArg), sharedResults()
    {
        assert(this->size > 0);

        if (shareResults)
        {
            this->sharedResults.reset(new SharedResults{});
        }
    }
    virtual ~Team() {}

    virtual std::string getInnerName() = 0;

    std::shared_ptr<SharedResults> getSharedResults()
    {
        return this->sharedResults;
    }

    virtual ContestResult runContest(ContestInput const & contest) = 0;
    std::string getXname() { return this->getSharedResults() ? "X" : ""; }
    virtual std::string getTeamName() { return this->getInnerName() + this->getXname() + "<" + std::to_string(this->size) + ">"; }
    uint32_t getSize() const { return this->size; }

private:
    uint32_t size;
    std::shared_ptr<SharedResults> sharedResults;
};

class TeamSolo : public Team
{
public:
    TeamSolo(uint32_t sizeArg): Team(1, false) {} // ignore size, don't share

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        ContestResult result;
        result.resize(contestInput.size());
        uint64_t idx = 0;

        rtimers::cxx11::DefaultTimer soloTimer("CalcCollatzSoloTimer");

        for(InfInt const & singleInput : contestInput)
        {
            auto scopedStartStop = soloTimer.scopedStart();
            result[idx] = calcCollatz(singleInput);
            ++idx;
        }
        return result;
    }

    virtual std::string getInnerName() { return "TeamSolo"; }
};

class TeamThreads : public Team
{
public:
    TeamThreads(uint32_t sizeArg, bool shareResults): Team(sizeArg, shareResults), createdThreads(0) {}

    template< class Function, class... Args >
    std::thread createThread(Function&& f, Args&&... args)
    {
        ++this->createdThreads;
        return std::thread(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    void resetThreads() { this->createdThreads = 0; } 
    uint64_t getCreatedThreads() { return this->createdThreads; }

private:
    uint64_t createdThreads;
};

class TeamNewThreads : public TeamThreads
{
public:
    TeamNewThreads(uint32_t sizeArg, bool shareResults): TeamThreads(sizeArg, shareResults) {}

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        this->resetThreads();
        ContestResult result = this->runContestImpl(contestInput);
        assert(contestInput.size() == this->getCreatedThreads());
        return result;
    }

    virtual ContestResult runContestImpl(ContestInput const & contestInput)
    {
        ContestResult r;
        r.resize(contestInput.size());
        uint64_t idx = 0;

        std::vector<std::promise<uint64_t>> promises(r.size());
        for (auto & promise : promises)
        {
            std::promise<uint64_t> p;
            promise = std::move(p);
        }

        std::vector<std::future<uint64_t>> futures(r.size());
        for (auto i = 0; i < futures.size(); ++i)
            futures[i] = promises[i].get_future();

        std::vector<std::thread> threads(r.size());
        uint64_t thread_cnt = 0;
        for (auto i = 0; i < contestInput.size(); ++i)
        {
            if (thread_cnt == getSize())
            {
                r[idx] = futures[idx].get();
                threads[idx].join();
                idx++;
                thread_cnt--;
            }
            std::thread t = createThread(
                    [&, n = contestInput[i], &promise = promises[i]]
                    {
                        if (this->getSharedResults())
                            promise.set_value(this->getSharedResults()->calcCollatzShare(n));
                        else
                            promise.set_value(calcCollatz(n));
                    });
            threads[i] = std::move(t);
            thread_cnt++;
            assert(thread_cnt <= getSize());
        }

        while (idx < r.size())
        {
            r[idx] = futures[idx].get();
            threads[idx].join();
            idx++;
        }

        return r;
    }

    virtual std::string getInnerName() { return "TeamNewThreads"; }
};

class TeamConstThreads : public TeamThreads
{
public:
    TeamConstThreads(uint32_t sizeArg, bool shareResults): TeamThreads(sizeArg, shareResults) {}

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        this->resetThreads();
        ContestResult result = this->runContestImpl(contestInput);
        assert(this->getSize() == this->getCreatedThreads());
        return result;
    }

    virtual ContestResult runContestImpl(ContestInput const & contestInput)
    {
        ContestResult r;
        r.resize(contestInput.size());
        uint32_t thread_cnt = getSize();

        std::vector<std::promise<uint64_t>> promises(r.size());
        for (auto & promise : promises)
        {
            std::promise<uint64_t> p;
            promise = std::move(p);
        }

        std::vector<std::future<uint64_t>> futures(r.size());
        for (auto i = 0; i < futures.size(); ++i)
            futures[i] = promises[i].get_future();

        std::vector<std::thread> threads;
        for (auto i = 0; i < thread_cnt; ++i)
        {
            auto sr = this->getSharedResults();
            std::thread t = createThread(
                    [contestInput, &promises, &sr, i, thread_cnt]
                    {
                        calcCollatzConstHelp(contestInput, promises, sr, i, thread_cnt);
                    });
            threads.push_back(std::move(t));
        }

        for (auto i = 0; i < r.size(); ++i)
            r[i] = futures[i].get();

        for (auto & thread : threads)
            thread.join();

        return r;
    }

    virtual std::string getInnerName() { return "TeamConstThreads"; }
};

class TeamPool : public Team
{
public:
    TeamPool(uint32_t sizeArg, bool shareResults): Team(sizeArg, shareResults), pool(sizeArg) {}

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        ContestResult r;
        r.resize(contestInput.size());
        uint64_t idx = 0;

        std::vector<std::future<uint64_t>> futures;

        while (idx < contestInput.size())
        {
            for (auto i = 0; i < getSize(); ++i, ++idx)
            {
                if (idx == contestInput.size())
                    break;

                if (this->getSharedResults())
                {
                    futures.push_back(pool.push(
                            [&, n = contestInput[idx]]
                            {
                                return this->getSharedResults()->calcCollatzShare(n);
                            }));
                }
                else
                {
                    futures.push_back(pool.push(
                            [&, n = contestInput[idx]]
                            {
                                return calcCollatz(n);
                            }));
                }
            }
        }

        for (auto i = 0; i < r.size(); ++i)
            r[i] = futures[i].get();

        return r;
    }

    virtual std::string getInnerName() { return "TeamPool"; }
private:
    cxxpool::thread_pool pool;
};

class TeamNewProcesses : public Team
{
public:
    TeamNewProcesses(uint32_t sizeArg, bool shareResults): Team(sizeArg, shareResults) {}

    virtual ContestResult runContest(ContestInput const & contestInput) {
        ContestResult r;
        r.resize(contestInput.size());
        pid_t pid;

        std::string in[r.size()];
        for (int i = 0; i < r.size(); ++i)
            in[i] = "/tmp/" + std::to_string(i);

        for (auto i = 0; i < contestInput.size(); ++i) {
            if (mkfifo(in[i].c_str(), 0755) == -1)
                exit(42);

            switch (pid = fork()) {
                case -1: // błąd fork()
                    exit(42);

                case 0: // proces potomny
                    execlp("./new_process", "./new_process", in[i].c_str(), NULL);
                    break;

                default: // proces macierzysty
                    break;
            }
        }

        for (int i = 0; i < r.size(); ++i)
            if (wait(0) == -1)
                exit(42);

        for (int i = 0; i < r.size(); ++i)
            if (unlink(in[i].c_str()))
                exit(42);

        return r;
    }

    virtual std::string getInnerName() { return "TeamNewProcesses"; }
};

class TeamConstProcesses : public Team
{
public:
    TeamConstProcesses(uint32_t sizeArg, bool shareResults): Team(sizeArg, shareResults) {}

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        ContestResult r;
        r.resize(contestInput.size());
        uint64_t process_cnt = getSize();
        pid_t pid;

        //TODO

        return r;
    }

    virtual std::string getInnerName() { return "TeamConstProcesses"; }
};

class TeamAsync : public Team
{
public:
    TeamAsync(uint32_t sizeArg, bool shareResults): Team(1, shareResults) {} // ignore size

    virtual ContestResult runContest(ContestInput const & contestInput)
    {
        ContestResult r;
        r.resize(contestInput.size());

        std::vector<std::future<uint64_t>> futures;

        for (auto & input : contestInput)
        {
            if (this->getSharedResults())
            {
                futures.push_back(std::async([&]
                {
                    return this->getSharedResults()->calcCollatzShare(input);
                }));
            }
            else
            {
                futures.push_back(std::async([&]
                {
                    return calcCollatz(input);
                }));
            }
        }

        for (auto i = 0; i < r.size(); ++i)
            r[i] = futures[i].get();

        return r;
    }

    virtual std::string getInnerName() { return "TeamAsync"; }
};

#endif