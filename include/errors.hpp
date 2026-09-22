#ifndef TEXTCODEC_ERRORS_HPP
#define TEXTCODEC_ERRORS_HPP

#include <string_view>

namespace textcodec {
    //process exit codes
    enum class Status : int {
        Ok = 0,           // success
        UsageError = 2,   // bad / missing command line args
        InputToLarge = 3, // input exceeded configured bound
        IoError = 4,      // failed to read stdin / write stdout
        DecodeError = 5   // input not valid Base64
    };

    constexpr int to_exit_code(Status s) noexcept {
        return static_cast<int> (s);
    }

    constexpr std::string_view describe(Status s) {
        switch (s) {
            case Status::Ok:           return "ok";
            case Status::UsageError:   return "usage error";
            case Status::InputToLarge: return "input eceeded max size";
            case Status::IoError:      return "input/output error";
            case Status::DecodeError:  return "invalid base64 input";
        }
        return "unknown error";
    }
} // namespace textcodec

#endif