#include "base64.hpp"
#include <array>

namespace textcodec::base64 {
    namespace {

        //64 output char, indexed by a 6-bit 0...63
        constexpr char kAlphabet[] = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ" // 0...25
            "abcdefghijklmnopqrstuvwxyz" // 26...51
            "0123456789+/";              // 52...63

        // padding char
        constexpr char kPad = '=';
        // sentinels for not part of alphabet
        constexpr std::int8_t kInvalid = -1; // not a base64 char
        constexpr std::int8_t kSkip = -2; // whitespace, ignore

        //build 256-entry byte -> value table once (called by decode_table)
        std::array<std::int8_t, 256> build_decode_table() {
            std::array<std::int8_t, 256> table{};
            for (auto& v : table) {
                v = kInvalid; //invalid by default
            }
            for (std::uint8_t i = 0; i < 64; ++i) {
                table[static_cast<std::uint8_t> (kAlphabet[i])] = i;
            }
            //ignore following
            table[static_cast<std::uint8_t>(' ')] = kSkip; // space
            table[static_cast<std::uint8_t>('\t')] = kSkip; // tab
            table[static_cast<std::uint8_t>('\r')] = kSkip; // CR
            table[static_cast<std::uint8_t>('\n')] = kSkip; // LF

            return table;
        }

        const std::array<std::int8_t, 256>& decode_table() {
            static const std::array<std::int8_t, 256> table = build_decode_table();
            return table;
        }
    } // namespace

    std::string encode(const Bytes& input) {
        std::string out;
        out.reserve(((input.size() + 2) / 3) * 4);

        std::size_t i = 0; 
        const std::size_t n = input.size();

        //process 3-byte groups
        while (i + 3 <= n) {
            const std::uint32_t triple = (static_cast<std::uint32_t> (input[i]) << 16) |    //byte0 -> bits23...16
                                         (static_cast<std::uint32_t> (input[i + 1]) << 8) | //byte1 -> bits15...8
                                         (static_cast<std::uint32_t> (input[i + 2]));       //byte2 -> bits7...0
            out.push_back(kAlphabet[(triple >> 18) & 0x3F]);  //top 6 bits
            out.push_back(kAlphabet[(triple >> 12) & 0x3F]); //next 6 bits
            out.push_back(kAlphabet[(triple >> 6) & 0x3F]);   //next 6 bits
            out.push_back(kAlphabet[triple & 0x3F]);
            i += 3;
        }

        //handle trailing 1 or 2 bytes with '='
        const std::size_t remaining = n - i;
        if (remaining == 1) {
            const std::uint32_t triple = static_cast<std::uint32_t> (input[i]) << 16; //only byte0
            out.push_back(kAlphabet[(triple >> 18) & 0x3F]); //top 6 bits
            out.push_back(kAlphabet[(triple >> 12) & 0x3F]); //next 2 bits (0-pad)
            out.push_back(kPad); //pad
            out.push_back(kPad); //pad
        } else if (remaining == 2) {
            const std::uint32_t triple = (static_cast<std::uint32_t> (input[i]) << 16) |    //byte0 -> bits23...16
                                         (static_cast<std::uint32_t> (input[i + 1]) << 8);  //byte1 -> bits15...8
            out.push_back(kAlphabet[(triple >> 18) & 0x3F]);  //top 6 bits
            out.push_back(kAlphabet[(triple >> 12) & 0x3F]); //next 6 bits
            out.push_back(kAlphabet[(triple >> 6) & 0x3F]);   //next 4 bits (0-pad)
            out.push_back(kPad); //pad
        }

        return out;
    }

    decodeResult decode(std::string_view input) {
        const auto& table = decode_table(); // byte -> 6-bit-value lookup
        decodeResult result; //default ok = false
        result.bytes.reserve((input.size() / 4) * 3 + 3); //~ upper bound

        std::uint32_t buffer = 0; //accumulates decoded 6-bit groups
        int bits = 0; //number of valid bits in buffer
        int pad = 0; //number of '=' seen

        for (const char c : input) {
            const auto uc = static_cast<std::uint8_t> (c); //treat current char as unsigned index

            //just count pad char
            if (c == kPad) {
                ++pad;
                continue;
            }

            const std::int8_t value = table[uc]; // lookup bytes meaning
            if (value == kSkip) {
                continue; // ignore whitespace 
            }

            // if char is outside alphabet, bail out, ok = false
            if (value == kInvalid) {
                return result;
            }
            // real data after pad is illegal, bail out, ok = false
            if (pad != 0) {
                return result;
            }
            //
            buffer = (buffer << 6) | static_cast<std::uint32_t> (value); //append 6 new bits
            bits += 6;
            //once we have a full byte, emit
            if (bits >= 8) {
                bits -= 8;
                result.bytes.push_back(static_cast<std::uint8_t> ((buffer >> bits) & 0xFF));
            }
        }

        //validate terminal, leftover bits must be padding-only and any bits not consumed into a byte are 0
        const int leftover = bits;
        if (leftover != 0) {
            //2 leftover bits -> one padding-carry byte consumed
            //4 leftover bits -> need 1 pad, leftover must be < 8
            //spare bits should be 0, if not reject
            if ((buffer & ((1u << leftover) - 1)) != 0) {
                return result;
            }
        }

        //enforce correct padding length, padding can only be 0, 1, or 2 char
        if (pad > 2) {
            return result; //more than two '==' impossible, ok=false
        }
        if (leftover == 6) {
            return result; //lone trailing char, always invalid
        }
        if (leftover == 4 && pad != 2) {
            return result; //one output byte needs '==', 2 data chars + '=='
        }
        if (leftover == 2 && pad != 1) {
            return result; //two output bytes need '=', 3 data chars + '='
        }
        if (leftover == 0 && pad != 0) {
            return result; //full group does not need padding
        }

        result.ok = true;
        return result;
    }

} //namespace base64