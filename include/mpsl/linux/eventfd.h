#ifndef MPSL_LINUX_EVENTFD
#define MPSL_LINUX_EVENTFD

#include "mpsl/posix.h"

namespace mpsl{

    inline ReadEventFDResult read_eventfd(int fd){
        ReadEventFDResult result;
        static_cast<ReadResult&>(result) = read(fd, result.count);
        return result;
    }
    inline WriteResult notify_eventfd(int fd){
        uint64_t r = 1;
        return write(fd, r);
    }
}

#endif
