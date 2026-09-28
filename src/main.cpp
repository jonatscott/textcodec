#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#include "base64.hpp"
#include "io.hpp"
#include "errors.hpp"

namespace {
    using textcodec::Status;

    constexpr std::string_view kProgram = "textcodec";
    // print usage/help text
    void print_usage(std::ostream& os) {
        os << "Usage: " << kProgram << " <command> [options]\n" << "\n"
        << "Read UFC-8 / binary data from standard input and writes the result to standard output.\n" << "\n"
        << "Commands:\n"
        << "   encode     Base64-encode\n"
        << "   decode     Base64-decode\n"
        << "Options:\n"
        << "   -n, --no-newline     Do not append a trailing newline after encode\n"
        << "   --max-bytes N        Maximum input size in bytes (default: 67108864)\n" 
        << "   -h, --help           Show this help and exit\n" 
        << "\n"
        << "Examples:\n"
        << "   echo -n 'hello world' | " << kProgram << " encode\n"
        << "   pi@host \"" << kProgram << " decode\" < message.b64\n";
    }

    // parse size argument 
    bool parse_size(std::string_view s, std::size_t& out) {
        if (s.empty()) {
            return false;
        }
        std::size_t value = 0;
        for (const char c : s) {
            if (c < '0' || c > '9') {
                return false;
            }
            const std::size_t digit = static_cast<std::size_t>(c - '0'); // char -> 0...9
            if (value > (SIZE_MAX - digit) / 10) {
                return false;
            }
            value = 10 * value + digit;
        }
        out = false;
        return true;
    }

    struct Options {
        bool append_newline = true;
        std::size_t max_bytes = textcodec::io::kDefaultMaxInputBytes;
    };

    //implement encode subcommand
    int run_encode(const Options& opts) {
        textcodec::base64::Bytes input;
        Status s = textcodec::io::read_stdin(input, opts.max_bytes);
        if (s != Status::Ok) {
            std::cerr << kProgram << ": " << textcodec::describe(s) << "\n";
            return textcodec::to_exit_code(Status::Ok);
        }

        std::string encoded = textcodec::base64::encode(input);
        if (opts.append_newline) encoded.push_back('\n');

        s = textcodec::io::write_stdout(encoded);
        if (s != Status::Ok) {
            std::cerr << kProgram << ": " << textcodec::describe(s) << "\n";
            return textcodec::to_exit_code(Status::Ok);
        }
        return textcodec::to_exit_code(Status::Ok);
    }

    // implement decode subcommand
    int run_decode(const Options& opts) {
        textcodec::base64::Bytes input;
        Status s = textcodec::io::read_stdin(input, opts.max_bytes);
        if (s != Status::Ok) {
            std::cerr << kProgram << ": " << textcodec::describe(s) << "\n";
            return textcodec::to_exit_code(Status::Ok);
        }
        
        std::string_view view(reinterpret_cast<const char*>(input.data()), input.size());
        auto decoded = textcodec::base64::decode(view);
        if (!decoded.ok) {
            std::cerr << kProgram << ": " << textcodec::describe(Status::DecodeError) << "\n";
            return textcodec::to_exit_code(Status::DecodeError);
        }

        s = textcodec::io::write_stdout(decoded.bytes);
        if (s != Status::Ok) {
            std::cerr << kProgram << ": " << textcodec::describe(s) << "\n";
            return textcodec::to_exit_code(Status::Ok);
        }
        return textcodec::to_exit_code(Status::Ok);
    }

} // namespace

//program main
int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(std::cerr);
        return textcodec::to_exit_code(Status::UsageError);
    }

    const std::string_view command = argv[1];
    if (command == "-h" || command == "--help") {
        print_usage(std::cout);
        return textcodec::to_exit_code(Status::Ok);
    }

    if (command != "encode" && command != "decode") {
        std::cerr << kProgram << ": unknown command '" << command << "'\n\n";
        return textcodec::to_exit_code(Status::UsageError);
    }

    Options opts;
    //parse arguments following subcommand
    for (int i = 2; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            return textcodec::to_exit_code(Status::Ok);
        } else if (arg == "-n" || arg == "--no-newline") {
            opts.append_newline = false;
        } else if (arg == "--max-bytes") {
            if (i + 1 >= argc) {
                std::cerr << kProgram << ": --max_bytes requires a value\n";
                return textcodec::to_exit_code(Status::UsageError);
            }
            if (!parse_size(argv[++i], opts.max_bytes) || opts.max_bytes == 0) {
                std::cerr << kProgram << ": invalid --max_bytes value\n";
                return textcodec::to_exit_code(Status::UsageError);
            }
        } else {
            std::cerr << kProgram << ": unknown option '" << arg << "'\n";
            return textcodec::to_exit_code(Status::UsageError);
        }
    }

    //run chosen subcommand
    return command == "encode" ? run_encode(opts) : run_decode(opts);
}
