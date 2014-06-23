#ifndef MPSL_POSIX_H
#define MPSL_POSIX_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

#include "mpsl/types.h"
#include "mpsl/iovec.h"

//basic operations
namespace mpsl{

    struct OpenResult : public BaseResult{
        int fd;
        inline int operator*(void) const{
            return fd;
        }
        inline OpenResult(): BaseResult(), fd(0){}
        inline OpenResult(bool success, int errnum, int fd): BaseResult(success, errnum), fd(fd){}
    };

    inline OpenResult open(const char* path, int flags){
        int lerrno = 0;
        int fd = ::open(path, flags);
        if(fd == -1){
            lerrno = errno;
        }
        return OpenResult(fd != -1, lerrno, fd);
    }

    inline OpenResult open(const char* path, int flags, mode_t mode){
        int lerrno = 0;
        int fd = ::open(path, flags, mode);
        if(fd == -1){
            lerrno = errno;
        }
        return OpenResult(fd != -1, lerrno, fd);
    }

    inline OpenResult open(const std::string &path, int flags){
        return safe_open(path.c_str(), flags);
    }

    inline OpenResult open(const std::string &path, int flags, mode_t mode){
        return safe_open(path.c_str(), flags, mode);
    }

    struct CloseResult : BaseResult{
        int result;
        inline CloseResult():BaseResult(), result(0){}
        inline CloseResult(int result, int errnum): BaseResult(result != 0, errnum), result(result){}
        inline int operator*(void) const{
            return result;
        }
    };

    inline CloseResult close(int fd){
        if(fd == -1){
            return CloseResult(true, 0);
        }
        int lerrno = 0;
        int result = ::close(fd);
        if(result == -1){
            lerrno = errno;
        }
        return CloseResult(result == 0, lerrno);
    }

    struct WriteResult : public BaseResult{
        size_t nwritten;
        inline size_t operator*(void) const{
            return nwritten;
        }
        inline WriteResult():BaseResult(), nwritten(0){}
        inline WriteResult(bool success, int errnum, size_t nwritten): BaseResult(success, errnum), nwritten(nwritten){}
    };

    //ensures (will not return) until min_count bytes have been written unless special non-recoverable circumstances have occured
    inline WriteResult write_some(int fd, const void *_buf, size_t max_count, size_t min_count){
        const char *buf = (const char *) _buf;
        int lerrno = 0;
        size_t total = 0;
        while(total < min_count) {
            int written = ::write(fd, (void *)buf, max_count - total);
            if (written == -1){
                lerrno = errno;
                if(lerrno == EINTR){
                    continue;
                }
                break;
            }else if(written == 0){
                lerrno = ENOSPC;
                break;
            }
            buf += written;
            total += written;
        }
        return WriteResult(total >= min_count, lerrno, total);
    }

    inline WriteResult write_all(int fd, const void *buf, size_t max_count){
        return write_some(fd, buf, max_count, max_count);
    }

    struct ReadResult : public BaseResult{
        size_t nread;
        bool m_eof;
        inline size_t operator*(void) const{
            return nread;
        }
        inline ReadResult():BaseResult(), nread(0), m_eof(false){}
        inline ReadResult(bool success, bool eof, int errnum, size_t nread): BaseResult(success, errnum), nread(nread), m_eof(eof){}
        inline bool eof(){//end of stream
            return m_eof;
        }
    };

    inline ReadResult read_some(int fd, void *_buf, size_t max_count, size_t min_count){
        char *buf = (char *) _buf;
        int lerrno = 0;
        size_t total = 0;
        bool eof = false;
        for(;;){
            size_t buffer_remaining = max_count - total;
            if(buffer_remaining > 0){
                int nread = ::read(fd, (void *) buf, buffer_remaining);
                if(nread == -1){
                    lerrno = errno;
                    if(lerrno == EINTR){
                        continue;
                    }
                    break;
                }else if(buffer_remaining > 0 && nread == 0){
                    eof = true;
                    break;
                }

                buf += nread;
                total += nread;
                if(total >= min_count){
                    break;
                }
            }else{
                break;
            }
        }
        return ReadResult(total >= min_count, eof, lerrno, total);
    }

    inline ReadResult read_all(int fd, void *buf, size_t count){
        return read_some(fd, buf, count, count);
    }

}

//vector extensions
namespace mpsl{
    struct IOVecWriteResult : public WriteResult{
        iovec_inplace_iterator iterator;//iterator to the current iovec sequence
        inline IOVecWriteResult():WriteResult(), iterator(){}
        inline IOVecWriteResult(bool success, int errnum, size_t written, iovec_inplace_iterator iterator): WriteResult(success, errnum, written), iterator(iterator){}
    };

    inline IOVecWriteResult write_all_inplace(int fd, struct iovec *iov, const size_t iovcnt){
        iovec_inplace_iterator iov_it(iov, iovcnt);
        int lerrno = 0;
        size_t running_total = 0;

        for(;!iov_it.eov();){
            assert(iov_it.iov_remaining() <= iovcnt);
            assert(iov_it.head() >= iov && iov_it.head() <= iov_it.end());
            int written = ::writev(fd, iov_it.head(), iov_it.iov_remaining());
            if(written == -1){
                lerrno = errno;
                if(lerrno == EINTR){
                    continue;
                }
                break;
            }else if(written == 0){
                lerrno = ENOSPC;
                break;
            }
            running_total += written;
            iov_it.advance((size_t) written);
        }
        const bool success = iov_it.eov();
        return IOVecWriteResult(success, lerrno, running_total, iov_it);
    }

    inline WriteResult write_all(int fd, const BufferSet &buffer){
        std::vector<struct iovec> iov(buffer.iov, buffer.iov + buffer.count);
        return write_all_inplace(fd, iov.data(), buffer.count);
    }

    template<size_t N>
    inline WriteResult write_all(int fd, const std::array<struct iovec, N> &iov){
        std::array<struct iovec, N> iov_copy = iov;
        return write_all_inplace(fd, iov_copy.data(), iov_copy.size());
    }

    template<typename... Args>
    inline WriteResult write(int fd, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return write_all_inplace(fd, buffers.data(), buffers.size());
    }

    struct IOVecReadResult : public ReadResult{
        iovec_inplace_iterator iterator;//iterator to the current iovec sequence
        inline IOVecReadResult():ReadResult(), iterator(){}
        inline IOVecReadResult(bool success, bool eof, int errnum, size_t nread, iovec_inplace_iterator iterator): ReadResult(success, eof, errnum, nread), iterator(iterator){}
    };

    inline ReadResult read_all_inplace(int fd, struct iovec *iov, const size_t iovcnt){
        iovec_inplace_iterator iov_it(iov, iovcnt);

        int lerrno = 0;
        bool eof = false;
        size_t running_total = 0;
        for(;!iov_it.eov();){
            assert(iov_it.iov_remaining() <= iovcnt);
            assert(iov_it.head() >= iov && iov_it.head() <= iov_it.end());
            int nread = ::readv(fd, iov_it.head(), iov_it.iov_remaining());
            if(nread == -1){
                lerrno = errno;
                if(lerrno == EINTR){
                    continue;
                }
                break;
            }else if(nread == 0 && iov_it.any_bytes_remaining()){
                eof = true;
                break;
            }
            running_total += nread;
            iov_it.advance((size_t) nread);
        }
        const bool success = iov_it.eov();
        return IOVecReadResult(success, eof, lerrno, running_total, iov_it);
    }

    inline ReadResult read_all(int fd, BufferSet &buffer){
        struct iovec *iov = (struct iovec *) alloca(buffer.count * sizeof(struct iovec));
        std::copy(buffer.iov, buffer.iov + buffer.count, iov);
        return read_all_inplace(fd, iov, buffer.count);
    }

    template<int N>
    inline ReadResult read_all(int fd, std::array<struct iovec, N> &ios){
        std::array<struct iovec, N> ios_copy = ios;
        return read_all_inplace(fd, ios_copy.data(), ios_copy.size());
    }

    template<typename... Args>
    inline ReadResult read(int fd, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return read_all_inplace(fd, buffers.data(), buffers.size());
    }
}

#endif
