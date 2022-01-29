#ifndef COLLATZ_HPP
#define COLLATZ_HPP

#include <assert.h>

#include "lib/rtimers/cxx11.h"
#include "lib/pool/cxxpool.h"

#include "contest.h"
#include "sharedresults.h"

inline uint64_t calcCollatz(InfInt n)
{
    // It's ok even if the value overflow
    uint64_t count = 0;
    assert(n > 0);

    while (n != 1)
    {
        ++count;
        if (n % 2 == 1)
        {
            n *= 3;
            n += 1;
        }
        else
        {
            n /= 2;
        }            
    }

    return count;
}

inline uint64_t calcCollatzShare(InfInt n, const std::shared_ptr<SharedResults> & sr)
{
    if (sr->getResults().find(n) != sr->getResults().end())
    {
        return sr->getResults().at(n);
    }

    // It's ok even if the value overflow
    uint64_t count = 0;
    assert(n > 0);

    while (n != 1)
    {
        ++count;
        if (n % 2 == 1)
        {
            n *= 3;
            n += 1;
        }
        else
        {
            n /= 2;
        }
    }

    sr->addResult(n, count);
    return count;
}

void calcCollatzConstHelp(const ContestInput & input,
                          std::vector<std::promise<uint32_t>> & promises,
                          const std::shared_ptr<SharedResults> & sr,
                          uint64_t idx,
                          uint64_t thread_cnt)
{
    for (uint64_t i = idx; i < input.size(); i += thread_cnt)
        promises[i].set_value(sr ? calcCollatzShare(input[i], sr) : calcCollatz(input[i]));
}

#endif