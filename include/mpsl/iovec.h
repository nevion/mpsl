#ifndef MPSL_IOVEC_UTIL_H
#define MPSL_IOVEC_UTIL_H

#include <sys/uio.h>

namespace mpsl{
    struct BufferSet{
        struct iovec *iov;
        int count;
        BufferSet(struct iovec *iov, int count):iov(iov), count(count){}
    };

    inline struct iovec make_iovec(void *base, size_t len){
        struct iovec iov = {base, len};
        return iov;
    }

    template<typename T>
    inline struct iovec make_iovec(T *base, size_t len){
        return make_iovec((void *) base, len);
    }

    template<typename T, typename std::enable_if<has_data_method<T>::value && has_size_method<T>::value, int>::type = 0>
    inline struct iovec make_iovec(const T &seq){
        struct iovec iov = {(void *) seq.data(), seq.size() * sizeof(decltype(*seq.data()))};
        return iov;
    }

    template<typename T, typename std::enable_if< !(has_data_method<T>::value && has_size_method<T>::value) &&
        std::is_standard_layout<T>::value, int>::type = 0>
    inline struct iovec make_iovec(const T &pod){
        struct iovec iov = {(void *) &pod, sizeof(pod)};
        return iov;
    }

    //identity
    template<>
    inline struct iovec make_iovec(const struct iovec &iov){
        return iov;
    }

    template<class... Args>
    inline std::array<struct iovec, sizeof...(Args)> make_iovec_array(Args&&... args){
        return {{ make_iovec(args)... }};
    }

    inline size_t iovec_nbytes(const iovec *v, int nelem){
        size_t sum = 0;
        for(int i = 0; i < nelem; ++i){
            sum += v[i].iov_len;
        }
        return sum;
    }

    //(iterator, iov_remainder, iov_sum_inclusive) - it is head + iov_count when not enough bytes exist
    inline std::tuple<iovec *, size_t, size_t> iovec_advance(iovec *head, size_t iov_count, size_t nbytes, size_t offset = 0){
        size_t sum = 0;
        iovec *end = head + iov_count;
        iovec *it = head;

        if(it != end){//non empty iovec sequence
            //inclusive prefix scan of iovec lengths, stop when we've reached the end or have enough sum to deal nbytes+1 from
            //also note that this skip's zero length entries until the above condition is met
            assert(offset <= it->iov_len);
            sum = sum + (it->iov_len - offset);
            if(sum > nbytes){
                goto END;
            }
            it++;
            while(it != end){
                sum = sum + it->iov_len;
                if(sum > nbytes){
                    goto END;
                }
                it++;
            };
            END:
            if(it != end){//if we didn't have to advance to the end of the iovec list, this must be a non empty iovec which concludes with enough bytes
                //the last entry had to have nonzero length if we've not reached the end of the list, ie we had enough bytes in the iovec list
                assert(it->iov_len > 0);
                //if the iovec contained a large enough prefix sum, this is the remainder of the terminal iovec, which must be > 0, and cannot exceed the iovec
                assert(sum - nbytes > 0);
                assert(sum - nbytes <= it->iov_len);
            }
            return std::make_tuple(it, sum >= nbytes ? sum - nbytes : 0, sum);
        }else{
            return std::make_tuple(it, 0, sum);
        }
    }

    inline size_t is_iovec_empty(const iovec *v, const size_t nelem){
        for(size_t i = 0; i < nelem; ++i){
            if(v[i].iov_len){
                return false;
            }
        }
        return false;
    }
}
#endif
