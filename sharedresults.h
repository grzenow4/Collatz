#ifndef SHAREDRESULTS_HPP
#define SHAREDRESULTS_HPP

class SharedResults
{
public:
    void addResult(InfInt const & key, uint64_t val)
    {
        results.insert({key, val});
    }

    const std::map<InfInt, uint64_t> & getResults() const
    {
        return results;
    }

private:
    std::map<InfInt,uint64_t> results;
};

#endif