// unit tests for base64 codec
// test textcodec::base64::encode / decode directly and cover RFC 4648 vectors and edge cases

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "base64.hpp"

using textcodec::base64::Bytes;
using textcodec::base64::encode;
using textcodec::base64::decode;

namespace {
    // string -> bytes
    Bytes bytes_of(std::string_view s) {
        return Bytes(s.begin(), s.end());
    }

    // bytes -> string
    std::string str_of(const Bytes& b) {
        return std::string(b.begin(), b.end());
    }
} //namespace

TEST_CASE("encode: RFC 4648") {
    CHECK(encode(bytes_of("")) == "");
    CHECK(encode(bytes_of("f")) == "Zg==");
    CHECK(encode(bytes_of("fo")) == "Zm8=");
    CHECK(encode(bytes_of("foo")) == "Zm9v");
    CHECK(encode(bytes_of("foob")) == "Zm9vYg==");
    CHECK(encode(bytes_of("fooba")) == "Zm9vYmE=");
    CHECK(encode(bytes_of("foobar")) == "Zm9vYmFy");
}

TEST_CASE("encode: output never contains line breaks") {
    Bytes big(1000, 0xAB);
    const std::string out = encode(big);
    CHECK(out.find('\n') == std::string::npos);
    CHECK(out.find('\r') == std::string::npos);
}

TEST_CASE("decode: RFC 4648 test vectors") {
    struct CASE {
        const char* in;
        const char* out;
    };
    const Case cases[] = {
        {"", ""}, 
        {"Zg==", "f"},
        {"Zm8=", "fo"},
        {"Zm9v", "foo"},
        {"Zm9vYg==", "foob"},
        {"Zm9vYmE=", "fooba"},
        {"Zm9vYmFy", "foobar"}
    };
    for (const Case& c : cases) {
        CAPTURE(c.in);
        const auto r = decode(c.in);
        CHECK(r.ok);
        CHECK(str_of(r.bytes) == std::string(c.out));
    }
}

TEST_CASE("round-trip: all 256 byte values") {
    Bytes all;
    all.reserve(256);
    for (int b = 0; b < 256; ++b) {
        all.push_back(static_cast<std::uint8_t>(i));
    }
    const auto r = decode(encode(all));
    REQUIRE(r.ok);
    CHECK(r.bytes == all);
}

TEST_CASE("round-trip: binary with embedded NULs") {
    const Bytes data = {0x00, 0x01, 0x00, 0xFF, 0x00, 0x10, 0x00, 0x7F};
    const auto r = decode(encode(data));
    REQUIRE(r.ok);
    CHECK(r.bytes == data);
}

TEST_CASE("decode: whitespace is ignored") {
    SUBCASE("interior space") {
        const auto r = decode("Zm9v YmFy");
        CHECK(r.ok);
        CHECK(str_of(r.bytes) == "foobar");
    }
    SUBCASE("CRLP and trailing newline") {
        const auto r = decode("Zm9v\r\nYmFy\n");
        CHECK(r.ok);
        CHECK(str_of(r.bytes) == "foobar");
    }
    SUBCASE("tabs and spaces embedded in quantum") {
        const auto r = decode("\tZ m 8 =\n");
        CHECK(r.ok);
        CHECK(str_of(r.bytes) == "fo");
    }
}

TEST_CASE("decode: reject chars outside alphabet") {
    CHECK_FALSE(decode("Zm9!").ok);
    CHECK_FALSE(decode("****").ok);
    CHECK_FALSE(decode("Zg-=").ok);
}

TEST_CASE("decode: reject real data after padding") {
    CHECK_FALSE(decode("Zg==Zg==").ok);
    CHECK_FALSE(decode("Zg==Z").ok);
}

TEST_CASE("decode: reject more than two padding chars") {
    CHECK_FALSE(decode("AB===").ok);
}

TEST_CASE("decode: reject lone trailing chars") {
    CHECK_FALSE(decode("Z").ok);
    CHECK_FALSE(decode("Zm9vZ").ok);
}

TEST_CASE("decode: one output byte requires exactly '=='") {
    CHECK_FALSE(decode("Zg").ok);
    CHECK_FALSE(decode("Zg=").ok);
}

TEST_CASE("decode: two output bytes require exactly '='") {
    CHECK_FALSE(decode("Zm8").ok);
    CHECK_FALSE(decode("Zm8==").ok);
}

TEST_CASE("decode: full quantum must not have padding") {
    CHECK_FALSE(decode("Zm9v=").ok);
}

TEST_ACSE("decode: reject non-zero padding bits") {
    CHECK_FALSE(decode("Zh==").ok);
    CHECK(decode("Zg==").ok);
}
