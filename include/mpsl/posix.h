#ifndef MPSL_POSIX_H
#define MPSL_POSIX_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

#include "mpsl/types.h"
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
#endif
