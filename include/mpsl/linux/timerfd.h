#ifndef MPSL_LINUX_TIMERFD
#define MPSL_LINUX_TIMERFD

#include <sys/timerfd.h>

#include "mpsl/posix.h"

namespace mpsl{

struct ReadTimerFDResult : public ReadResult{
    uint64_t expirations;
    inline ReadTimerFDResult():ReadResult(){}
    inline ReadTimerFDResult(bool success, bool eof, int errnum, size_t nread, uint64_t expirations): ReadResult(success, eof, errnum, nread), expirations(expirations){}
};

inline ReadTimerFDResult read_timerfd(int fd){
    ReadTimerFDResult result;
    static_cast<ReadResult&>(result) = read(fd, result.expirations);
    return result;
}

struct TimerFDSetTimeResult : BaseResult{
    struct itimerspec old_value;
    inline TimerFDSetTimeResult(bool success, const struct itimerspec &old_value): BaseResult(success, errno), old_value(old_value){}
    inline struct itimerspec operator*(void) const{
        return old_value;
    }
};

inline TimerFDSetTimeResult timerfd_settime(int fd, int flags, const struct itimerspec &itimerspec){
    struct itimerspec old_value;
    int return_code = ::timerfd_settime(fd, flags, &itimerspec, &old_value);
    return TimerFDSetTimeResult(return_code == 0, old_value);
}

}
#endif
