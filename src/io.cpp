#include "io.hpp"
#include <array>
#include <cstdio> 

#if defined(_WIN32) // following needed for windows
#include <fcntl.h> //for _O_BINARY
#include <io.h>    // for _setmode / _fileno
#endif

namespace textcodec::io {
    namespace {
        
        void ensure_binary_stream() {
            // not needed for Raspberry Pi OS but helpful if built elsewhere
            #if defined(_WIN32)
                _setmode(_fileno(stdin), _O_BINARY);
                _setmode(_fileno(stdout), _O_BINARY);
            #endif
        }

    } // namespace

    Status read_stdin(base64::Bytes& out, std::size_t max_bytes) {
        ensure_binary_stream();

        out.clear();

        constexpr std::size_t kChunk = 64 * 1024; // read in 64 KiB
        std::array<std::uint8_t, kChunk> buffer{};

        while (true) { //loop until end of file 
            const std::size_t got = std::fread(buffer.data(), 1, buffer.size(), stdin);

            if (got > 0) {
                if (got > max_bytes - out.size()) {
                    //reject if to big
                    return Status::InputToLarge;
                }
                // append
                out.insert(out.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(got));
            }

            if (got < buffer.size()) {
                if (std::ferror(stdin)) {
                    return Status::IoError;
                }
                break; // EOF reached
            }
        }

        return Status::Ok;
    }

    Status write_stdout(const base64::Bytes& data) {
        ensure_binary_stream();

        if (!data.empty()) {
            const std::size_t written = std::fwrite(data.data(), 1, data.size(), stdout);
            if (written != data.size()) {
                return Status::IoError;
            }
        }

        return std::fflush(stdout) == 0 ? Status::Ok : Status::IoError;
    }

    Status write_stdout(std::string_view data) {
        ensure_binary_stream();

        if (!data.empty()) {
            const std::size_t written = std::fwrite(data.data(), 1, data.size(), stdout);
            if (written != data.size()) {
                return Status::IoError;
            }
        }

        return std::fflush(stdout) == 0 ? Status::Ok : Status::IoError;
    }
} // namespace textcodec::io