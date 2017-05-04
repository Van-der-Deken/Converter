#ifndef MINGWSTOPWATCH_H_INCLUDED
#define MINGWSTOPWATCH_H_INCLUDED

#include <sys/time.h>
#include "IStopwatch.h"

namespace conv
{
    class MinGWStopwatch : public IStopwatch
    {
        public:
            void start() override;
            void lap() override;
        private:
            timeval timeValue;
    };
}

#endif // MINGWSTOPWATCH_H_INCLUDED
