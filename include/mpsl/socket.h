#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdexcept> //only used in to_string convenience functions!

#include "mpsl/posix.h"

namespace mpsl{

    struct SocketResult : public BaseResult{
        int fd;
        inline int operator*(void) const{
            return fd;
        }
        inline SocketResult(): BaseResult(), fd(0){}
        inline SocketResult(bool success, int errnum, int fd): BaseResult(success, errnum), fd(fd){}
    };

    inline SocketResult socket(const int family, const int type, const int protocol = 0){
        int lerrno = 0;
        int fd = ::socket(family, type, protocol);
        if(fd == -1){
            lerrno = errno;
        }
        return SocketResult(fd != -1, lerrno, fd);
    }

    struct BindResult : public BaseResult{
        inline BindResult(): BaseResult(){}
        inline BindResult(bool success, int errnum): BaseResult(success, errnum){}
    };

    inline BindResult bind(const int socket, const sockaddr *address, size_t address_len){
        int lerrno = 0;
        int result = ::bind(socket, address, address_len);
        if(result == -1){
            lerrno = errno;
        }
        return BindResult(result != -1, lerrno);
    }

    inline BindResult bind(const int socket, const sockaddr_storage &address, size_t address_len){
        return bind(socket, (struct sockaddr *) &address, address_len);
    }

    template<typename sockaddr_t>
    inline BindResult bind(const int socket, const sockaddr_t &address){
        return bind(socket, (struct sockaddr *) &address, sizeof(sockaddr_t));
    }

    struct ListenResult : public BaseResult{
        inline ListenResult(): BaseResult(){}
        inline ListenResult(bool success, int errnum): BaseResult(success, errnum){}
    };

    inline
    ListenResult listen(const int socket, const int backlog = 10){
        int lerrno = 0;
        int result = ::listen(socket, backlog);
        if(result == -1){
            lerrno = errno;
        }
        return ListenResult(result != -1, lerrno);
    }

    struct ConnectResult : public BaseResult{
        inline ConnectResult(): BaseResult(){}
        inline ConnectResult(bool success, int errnum): BaseResult(success, errnum){}
    };

    inline ConnectResult connect(const int socket, const sockaddr *address, size_t address_len){
        int lerrno = 0;
        int result = ::connect(socket, address, address_len);
        if(result == -1){
            lerrno = errno;
        }
        return ConnectResult(result != -1, lerrno);
    }

    inline ConnectResult connect(const int socket, const sockaddr_storage &address, size_t address_len){
        return connect(socket, (struct sockaddr *) &address, address_len);
    }

    template<typename sockaddr_t>
    ConnectResult connect(const int socket, const sockaddr_t addr){
        return connect(socket, (const struct sockaddr *)&addr, sizeof(sockaddr_t));
    }

    struct RecvResult : public BaseResult{
        size_t read;
        size_t length;
        inline size_t operator*(void) const{
            return read;
        }
        inline bool eof(){//end of stream
            return read == 0 && length > 0;
        }
        inline RecvResult():BaseResult(), read(0), length(0){}
        inline RecvResult(bool success, int errnum, size_t read, size_t length): BaseResult(success, errnum), read(read), length(length){}
    };

    inline RecvResult recv(int fd, void *buf, size_t length, int flags){
        int lerrno = 0;
        int nread = ::recv(fd, (void *) buf, length, flags);
        if (nread == -1){
            lerrno = errno;
        }
        return RecvResult(nread >= 0, lerrno, nread, length);
    }

    struct RecvFromResult : public BaseResult{
        size_t input_size;
        size_t read;
        sockaddr_storage sockaddr;
        socklen_t sockaddr_len;
        inline size_t operator*(void) const{
            return read;
        }
        inline bool all() const{
            return read >= input_size;
        }
        inline bool eof(){//end of stream
            return read == 0 && input_size > 0;
        }
        inline operator sockaddr_un() const{
            return sockaddr_un(*reinterpret_cast<const struct sockaddr_un *>(&sockaddr));
        }
        inline operator sockaddr_in() const{
            return sockaddr_in(*reinterpret_cast<const struct sockaddr_in *>(&sockaddr));
        }
        inline operator sockaddr_in6() const{
            return sockaddr_in6(*reinterpret_cast<const struct sockaddr_in6 *>(&sockaddr));
        }
        inline RecvFromResult():BaseResult(), input_size(0), read(0), sockaddr({}){}
        inline RecvFromResult(bool success, int errnum, size_t input_size, size_t read, sockaddr_storage sockaddr, socklen_t sockaddr_len): BaseResult(success, errnum), input_size(input_size), read(read), sockaddr(sockaddr), sockaddr_len(sockaddr_len){}
    };

    inline RecvFromResult recvfrom(int fd, void *buf, size_t length, int flags){
        int lerrno = 0;
        sockaddr_storage _sockaddr;
        socklen_t len = sizeof(_sockaddr);
        int nread = ::recvfrom(fd, (void *) buf, length, flags, (struct sockaddr *) &_sockaddr, &len);
        if (nread == -1){
            lerrno = errno;
        }
        return RecvFromResult(nread >= 0, lerrno, length, nread, _sockaddr, len);
    }

    struct RecvMsgResult : public BaseResult{
        size_t sizeof_args;
        size_t read;
        sockaddr_storage sockaddr;
        socklen_t sockaddr_len;
        int msg_flags;

        size_t controllen;
        inline size_t operator*(void) const{
            return read;
        }
        inline bool all() const{
            return read >= sizeof_args;
        }
        inline bool truncated() const{
            return msg_flags & MSG_TRUNC;
        }
        inline operator sockaddr_un() const{
            return sockaddr_un(*reinterpret_cast<const struct sockaddr_un *>(&sockaddr));
        }
        inline operator sockaddr_in() const{
            return sockaddr_in(*reinterpret_cast<const struct sockaddr_in *>(&sockaddr));
        }
        inline operator sockaddr_in6() const{
            return sockaddr_in6(*reinterpret_cast<const struct sockaddr_in6 *>(&sockaddr));
        }
        inline bool eof(){//end of stream
            return read == 0 && sizeof_args > 0;
        }

        inline RecvMsgResult():BaseResult(), sizeof_args(0), read(0), sockaddr({}), sockaddr_len(0), msg_flags(0){}
        inline RecvMsgResult(bool success, int errnum, size_t input_size, size_t read, sockaddr_storage sockaddr, socklen_t sockaddr_len, int msg_flags, size_t controllen): BaseResult(success, errnum), sizeof_args(input_size), read(read), sockaddr(sockaddr), sockaddr_len(sockaddr_len), msg_flags(msg_flags), controllen(controllen){}
    };

    inline RecvMsgResult recvmsgv(int fd, int flags, struct iovec *iov, const size_t iovcnt){
        sockaddr_storage _sockaddr;
        struct msghdr msg_header = {};
        msg_header.msg_name = &_sockaddr;
        msg_header.msg_namelen = sizeof(_sockaddr);
        msg_header.msg_iov = iov;
        msg_header.msg_iovlen = iovcnt;
        msg_header.msg_control = NULL;
        msg_header.msg_controllen = 0;
        int lerrno = 0;
        int nread = ::recvmsg(fd, &msg_header, flags);
        if (nread == -1){
            lerrno = errno;
        }
        return RecvMsgResult(nread >= 0, lerrno, iovec_nbytes(iov, iovcnt), nread, _sockaddr, msg_header.msg_namelen, msg_header.msg_flags, msg_header.msg_controllen);
    }

    template<typename... Args>
    inline RecvMsgResult recvmsg(int fd, int flags, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return unistd::recvmsgv(fd, flags, buffers.data(), buffers.size());
    }

    inline RecvMsgResult recvmsgv(int fd, int flags, void *ancillary_data, size_t nancillary_bytes, struct iovec *iov, const size_t iovcnt){
        sockaddr_storage _sockaddr;
        struct msghdr msg_header = {};
        msg_header.msg_name = &_sockaddr;
        msg_header.msg_namelen = sizeof(_sockaddr);
        msg_header.msg_iov = iov;
        msg_header.msg_iovlen = iovcnt;
        msg_header.msg_control = ancillary_data;
        msg_header.msg_controllen = nancillary_bytes;

        int lerrno = 0;
        int nread = ::recvmsg(fd, &msg_header, flags);
        if (nread == -1){
            lerrno = errno;
        }

        return RecvMsgResult(nread >= 0, lerrno, iovec_nbytes(iov, iovcnt), nread, _sockaddr, msg_header.msg_namelen, msg_header.msg_flags, msg_header.msg_controllen);
    }
    template<typename... Args>
    inline RecvMsgResult recvmsg_with_ancillary(int fd, int flags, void *ancillary_buffer, size_t ancillary_buffer_size, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return unistd::recvmsgv(fd, flags, ancillary_buffer, ancillary_buffer_size, buffers.data(), buffers.size());
    }

    struct SendToResult : public BaseResult{
        size_t written;
        inline size_t operator*(void) const{
            return written;
        }
        inline SendToResult():BaseResult(), written(0){}
        inline SendToResult(bool success, int errnum, size_t written): BaseResult(success, errnum), written(written){}
    };

    template<typename sockaddr_t>
    SendToResult sendto(int fd, const void *buf, size_t length, int flags, const sockaddr_t &sockaddr){
        int lerrno = 0;
        int nwritten = ::sendto(fd, buf, length, flags, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
        if (nwritten == -1){
            lerrno = errno;
        }
        return SendToResult((size_t) nwritten == length, lerrno, nwritten);
    }

    struct SendMsgResult : public BaseResult{
        size_t written;
        inline size_t operator*(void) const{
            return written;
        }
        inline SendMsgResult():BaseResult(), written(0){}
        inline SendMsgResult(bool success, int errnum, size_t written): BaseResult(success, errnum), written(written){}
    };

    inline
    SendMsgResult sendmsgv(int fd, const void *ancillary_data, size_t nancillary_bytes, const struct iovec *iov, const size_t iovcnt, int flags, const void *sockaddr, size_t sockaddr_len){
        int lerrno = 0;
        struct msghdr msg_header = {};
        msg_header.msg_name = (void *) sockaddr;
        msg_header.msg_namelen = sockaddr_len;
        msg_header.msg_iov = (struct iovec *) iov;
        msg_header.msg_iovlen = iovcnt;
        msg_header.msg_control = (void *) ancillary_data;
        msg_header.msg_controllen = nancillary_bytes;

        int nwritten = ::sendmsg(fd, &msg_header, flags);
        if (nwritten == -1){
            lerrno = errno;
        }
        return SendMsgResult((size_t) nwritten == iovec_nbytes(iov, (int) iovcnt), lerrno, nwritten);
    }

    template<typename sockaddr_t>
    SendMsgResult sendmsgv(int fd, const struct iovec *iov, const size_t iovcnt, int flags, const sockaddr_t *sockaddr){
        return unistd::sendmsgv(fd, nullptr, 0, iov, iovcnt, flags, sockaddr, sizeof(sockaddr_t));
    }

    template<typename sockaddr_t>
    SendMsgResult sendmsgv(int fd, const struct iovec *iov, const size_t iovcnt, int flags, const sockaddr_t &sockaddr){
        return unistd::sendmsgv(fd, nullptr, 0, iov, iovcnt, flags, &sockaddr, sizeof(sockaddr_t));
    }

    template<typename... Args>
    inline SendMsgResult sendmsg(int fd, int flags, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return unistd::sendmsgv(fd, nullptr, 0, buffers.data(), buffers.size(), flags, nullptr, 0);
    }

    template<typename... Args>
    inline SendMsgResult sendmsg_with_ancillary(int fd, int flags, const void *ancillary_data, const size_t nancillary_size, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return unistd::sendmsgv(fd, ancillary_data, nancillary_size, buffers.data(), buffers.size(), flags, nullptr, 0);
    }

    template<typename sockaddr_t, typename... Args>
    inline SendMsgResult sendmsg_to(int fd, int flags, const sockaddr_t &sockaddr, Args&&... pods){
        auto buffers = make_iovec_array(std::forward<Args>(pods)...);
        return unistd::sendmsgv(fd, nullptr, 0, buffers.data(), buffers.size(), flags, &sockaddr, sizeof(sockaddr_t));
    }

    inline struct sockaddr_un make_sockaddr_un(const std::string &path){
        sockaddr_un sockaddr = {};
        sockaddr.sun_family = AF_UNIX;
        std::copy(path.begin(), std::min(path.begin() + sizeof(sockaddr.sun_path), path.end()), sockaddr.sun_path);
        return sockaddr;
    }

    inline struct sockaddr_in make_sockaddr_in(const struct in_addr &addr, uint16_t port){
        sockaddr_in sockaddr = {};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);
        sockaddr.sin_addr = addr;
        return sockaddr;
    }

    inline struct sockaddr_in make_sockaddr_in(const uint32_t &addr, uint16_t port){
        struct in_addr _addr = {};
        _addr.s_addr = htons(addr);
        return make_sockaddr_in(_addr, port);
    }

    inline struct sockaddr_in6 make_sockaddr_in6(const struct in6_addr &addr, uint16_t port){
        sockaddr_in6 sockaddr = {};
        sockaddr.sin6_family = AF_INET6;
        sockaddr.sin6_port = htons(port);
        sockaddr.sin6_addr = addr;
        return sockaddr;
    }

    inline struct in_addr str2addr_in(const std::string &str){
        struct in_addr addr = {};
        ::inet_pton(AF_INET, str.c_str(), &addr);
        return addr;
    }

    inline struct in6_addr str2addr_in6(const std::string &str){
        struct in6_addr addr = {};
        ::inet_pton(AF_INET6, str.c_str(), &addr);
        return addr;
    }

    template<typename T>
    struct ResultT : BaseResult{
        int return_code;
        inline ResultT(T return_code): BaseResult(return_code >= 0, errno), return_code(return_code){}
        inline T operator*(void) const{
            return return_code;
        }
    };

    typedef ResultT<int> Result;

    template<typename T>
    struct GetSockOptResult : public BaseResult{
        T value;
        inline GetSockOptResult():BaseResult(){}
        inline GetSockOptResult(bool success, int errnum, T value): BaseResult(success, errnum), value(value){}
        inline T operator*(void) const{
            return value;
        }
    };

    template<typename T>
    GetSockOptResult<T> getsockopt(int sockfd, int level, int optname){
        T value;
        socklen_t length = sizeof(T);
        const int ret = ::getsockopt(sockfd, level, optname, &value, &length);
        const int lerrno = ret != 0 ? errno : 0;
        return GetSockOptResult<T>(ret == 0 && length == sizeof(T), lerrno, value);
    };

    struct SetSockOptResult : public BaseResult{
        inline SetSockOptResult():BaseResult(){}
        inline SetSockOptResult(bool success, int errnum): BaseResult(success, errnum){}
    };

    template<typename T>
    SetSockOptResult setsockopt(int sockfd, int level, int optname, const T &value){
        socklen_t length = sizeof(T);
        const int ret = ::setsockopt(sockfd, level, optname, &value, length);
        const int lerrno = ret != 0 ? errno : 0;
        return SetSockOptResult(ret == 0 && length == sizeof(T), lerrno);
    };

    inline std::string to_string(const struct in_addr &addr){
        std::string str('x', INET_ADDRSTRLEN);
        const char *result = ::inet_ntop(AF_INET, &addr, (char *) str.data(), str.size());
        if(result == nullptr){
            int lerrno = errno;
            throw std::runtime_error("error in inet_ntop: "+std::string(strerror(lerrno)));
        }
        return str;
    }

    inline std::string to_string(const struct in_addr6 &addr){
        std::string str('x', INET6_ADDRSTRLEN);
        const char *result = ::inet_ntop(AF_INET6, &addr, (char *) str.data(), str.size());
        if(result == nullptr){
            int lerrno = errno;
            throw std::runtime_error("error in inet_ntop: "+std::string(strerror(lerrno)));
        }
        return str;
    }
}
