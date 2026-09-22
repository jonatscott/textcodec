#ifndef TEXTCODEC_BASE64_HPP
#define TEXTCODEC_BASE64_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace textcodec::base64 {
    using Bytes = std::vector<std::uint8_t>;

    // encode an arbitrary byte sequence 
    std::string encode(const Bytes& input);

    //result of decode attempt
    struct decodeResult {
        bool ok = false; //valid or not
        Bytes bytes; //decoded bytes
    };

    decodeResult decode(std::string_view input);

} //namespace textcodec::base64



#endif