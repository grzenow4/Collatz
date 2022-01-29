#ifndef SHAREDRESULTS_HPP
#define SHAREDRESULTS_HPP

#include <shared_mutex>

class SharedResults
{
public:
    const std::map<InfInt, uint64_t> & getResults() const
    {
        return results;
    }

    uint64_t calcCollatzShare(InfInt n)
    {
        auto copy = n;

        if (results.find(n) != results.end())
        {
            return results.at(n);
        }

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

        std::unique_lock<std::shared_mutex> lock(mut);
        results.insert({copy, count});
        return count;
    }

private:
    std::shared_mutex mut;
    std::map<InfInt,uint64_t> results;
};

#endif