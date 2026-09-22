#ifndef TEXTCODEC_IO_HPP
#define TEXTCODEC_IO_HPP

#include <cstddef>
#include <string>

#include "base64.hpp"
#include "errors.hpp"

namespace textcodec::io {
    constexpr std::size_t kDefaultMaxInputBytes = 64ull * 1024 * 1024;

    // return Status::ok on success, bytes placed in out
    Status read_stdin(base64::Bytes& out, std::size_t max_bytes = kDefaultMaxInputBytes);

    // write raw bytes to standard output (binary-safe)
    Status write_stdout(const base64::Bytes& data); // for byte vectors
    Status write_stdout(std::string_view data);     // for text/strings

} // namespace textcodec::io

#endif