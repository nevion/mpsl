#ifndef MPSL_TYPES_H
#define MPSL_TYPES_H

#include <cerrno>

namespace mpsl{

    struct BaseResult{
        bool success;
        std::error_code error_code;
        inline explicit operator bool() const{
            return success;
        }
        inline std::string strerror() const{
            return error_code.message();
        }
        inline std::error_code code() const{ return error_code; }
        inline explicit operator int() const{
            return code().value();
        }
        inline bool operator==(const int evalue) const{
            return code().value() == evalue;
        }
        inline bool operator!=(const int evalue) const{
            return code().value() != evalue;
        }
        inline BaseResult():success(false), error_code(0, std::generic_category()){}
        inline BaseResult(bool success, int errnum):success(success), error_code(errnum, std::generic_category()){}
    };

}
#endif
