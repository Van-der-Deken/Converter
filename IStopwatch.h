#ifndef ISTOPWATCH_H_INCLUDED
#define ISTOPWATCH_H_INCLUDED

#include <stdint.h>
#include <vector>

namespace conv
{
    class IStopwatch
    {
        public:
            IStopwatch() {};
            virtual void start() = 0;
            virtual void lap() = 0;
            std::vector<uint64_t> getLaps() {return laps;};
            virtual ~IStopwatch() {};
        protected:
            uint64_t previous = 0;
            std::vector<uint64_t> laps;
    };
}

#endif // ISTOPWATCH_H_INCLUDED
