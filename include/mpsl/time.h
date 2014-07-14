#ifndef MPSL_TIME_H
#define MPSL_TIME_H

#include <time.h>

#include "mpsl/types.h"

namespace mpsl{

    inline uint64_t nanos_from_timespec(timespec ts){
        return uint64_t(ts.tv_sec) * uint64_t(1e9) + uint64_t(ts.tv_nsec);
    }
    inline timespec timespec_from_nanos(uint64_t x){
        return { time_t(x / uint64_t(1e9)), long(x % uint64_t(1e9))};
    }
    inline uint64_t usecs_from_timespec(timespec ts){
        return nanos_from_timespec(ts) / uint64_t(1e3);
    }
    inline timespec timespec_from_usecs(uint64_t x){
        return timespec_from_nanos(x * uint64_t(1e3));
    }
    inline uint64_t millis_from_timespec(timespec ts){
        return nanos_from_timespec(ts) / uint64_t(1e6);
    }
    inline timespec timespec_from_millis(uint64_t x){
        return timespec_from_nanos(x * uint64_t(1e6));
    }

    struct ClockGetTimeResult : BaseResult{
        struct timespec res;
        inline ClockGetTimeResult(bool success, const struct timespec &res): BaseResult(success, errno), res(res){}
        inline struct timespec operator*(void) const{
            return res;
        }
        inline uint64_t millis() const{
            return millis_from_timespec(res);
        }
        inline uint64_t usecs() const{
            return usecs_from_timespec(res);
        }
        inline uint64_t nanos() const{
            return nanos_from_timespec(res);
        }
    };

    inline ClockGetTimeResult clock_gettime(clockid_t clk_id){
        struct timespec res;
        int return_code = ::clock_gettime(clk_id, &res);
        return ClockGetTimeResult(return_code == 0, res);
    }

    struct ClockSetTimeResult : BaseResult{
        inline ClockSetTimeResult(bool success): BaseResult(success, errno){}
    };

    inline ClockSetTimeResult clock_settime_nanos(clockid_t clk_id, uint64_t nanos){
        struct timespec ts = timespec_from_nanos(nanos);
        int return_code = ::clock_settime(clk_id, &ts);
        return ClockSetTimeResult(return_code == 0);
    }
    inline ClockSetTimeResult clock_settime_usecs(clockid_t clk_id, uint64_t usecs){
        return clock_settime_nanos(clk_id, usecs * 1000);
    }
    inline ClockSetTimeResult clock_settime_millis(clockid_t clk_id, uint64_t millis){
        return clock_settime_usecs(clk_id, millis * 1000);
    }

    inline struct itimerspec make_itimerspec_nanos(uint64_t initial_nanos, uint64_t period_nanos){
        itimerspec itspec = {};
        itspec.it_value = timespec_from_nanos(initial_nanos);
        itspec.it_interval = timespec_from_nanos(period_nanos);
        return itspec;
    }
}
#endif
