#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

#include "utf.hpp"

using namespace utf;

namespace {
    template <typename T, size_t N>
    size_t elems(const T(&arr)[N]) { return N; }
}

TEST_CASE("traits/utf8/write_length", "Given a codepoint, compute its length if UTF-8 encoded") {
    typedef utf_traits<utf8> traits_t;

    CHECK(traits_t::write_length((codepoint_type)0x00) == 1);
    CHECK(traits_t::write_length((codepoint_type)0x61) == 1);
    CHECK(traits_t::write_length((codepoint_type)0x80) == 2);
    CHECK(traits_t::write_length((codepoint_type)0x07ff) == 2);
    CHECK(traits_t::write_length((codepoint_type)0x0800) == 3);
    CHECK(traits_t::write_length((codepoint_type)0xffff) == 3);
    CHECK(traits_t::write_length((codepoint_type)0x010000) == 4);
    CHECK(traits_t::write_length((codepoint_type)0x10ffff) == 4);

    SECTION("Code point in invalid range", "Code points in the range 0xd800-0xdfff are invalid") {
        CHECK(traits_t::write_length((codepoint_type)0xd7ff) == 3);
        CHECK(traits_t::write_length((codepoint_type)0xd800) == 0);
        CHECK(traits_t::write_length((codepoint_type)0xdabc) == 0);
        CHECK(traits_t::write_length((codepoint_type)0xdfff) == 0);
        CHECK(traits_t::write_length((codepoint_type)0xe000) == 3);
    }
    SECTION("Code point value too big", "Code points must not have values greater than 0x10ffff") {
        CHECK(traits_t::write_length((codepoint_type)0x110000) == 0);
        // largest that could theoretically be encoded as 4-byte utf8
        CHECK(traits_t::write_length((codepoint_type)0x1fffff) == 0);
        // smallest that would require 5 bytes
        CHECK(traits_t::write_length((codepoint_type)0x200000) == 0);
        // largest that could be encoded with 6 bytes
        CHECK(traits_t::write_length((codepoint_type)0x7fffffff) == 0);
    }
}

TEST_CASE("traits/utf8/read_length", "Length of char subsequence according to its leading byte") {
    typedef utf_traits<utf8> traits_t;
    typedef traits_t::codeunit_type codeunit_t;

    CHECK(traits_t::read_length((codeunit_t)0x00) == 1);
    CHECK(traits_t::read_length((codeunit_t)0x7f) == 1);
    CHECK(traits_t::read_length((codeunit_t)0xc2) == 2);
    CHECK(traits_t::read_length((codeunit_t)0xdf) == 2);
    CHECK(traits_t::read_length((codeunit_t)0xe0) == 3);
    CHECK(traits_t::read_length((codeunit_t)0xef) == 3);
    CHECK(traits_t::read_length((codeunit_t)0xf0) == 4);
    CHECK(traits_t::read_length((codeunit_t)0xf7) == 4);

    SECTION("5-byte sequence", "5-byte UTF-8 sequences are forbidden as per RFC 3629") {
        CHECK(traits_t::read_length((codeunit_t)0xf8) == 1);
        CHECK(traits_t::read_length((codeunit_t)0xfb) == 1);
    }
    SECTION("6-byte sequence", "6-byte UTF-8 sequences are forbidden as per RFC 3629") {
        CHECK(traits_t::read_length((codeunit_t)0xfc) == 1);
        CHECK(traits_t::read_length((codeunit_t)0xfd) == 1);
    }
    SECTION("Invalid byte value", "0xfe and 0xff are not valid UTF-8 bytes") {
        CHECK(traits_t::read_length((codeunit_t)0xfe) == 1);
        CHECK(traits_t::read_length((codeunit_t)0xff) == 1);
    }
    SECTION("Continuation byte", "Bytes of the form 10xxxxxx cannot begin a UTF-8 code unit subsequence") {
        CHECK(traits_t::read_length((codeunit_t)0x80) == 1);
        CHECK(traits_t::read_length((codeunit_t)0xbf) == 1);
    }
    SECTION("Other types", "Check that read_length works with other relevant codepoint types") {
        CHECK(traits_t::read_length((signed char)0xf0) == 4);
        CHECK(traits_t::read_length((unsigned char)0xf0) == 4);
        CHECK(traits_t::read_length((char)0xf0) == 4);
    }
}

TEST_CASE("traits/utf8/encode", "UTF-8 encode a single codepoint") {
    typedef utf_traits<utf8> traits_t;
    std::vector<char> buf;

    SECTION("null", "encode a null byte") {
        traits_t::encode(0x00, std::back_inserter(buf));

        REQUIRE(buf.size() == 1);
        CHECK(buf[0] == (char)0x00);
    }
    SECTION("1 byte", "encode a character as a single byte") {
        traits_t::encode(0x61, std::back_inserter(buf));

        REQUIRE(buf.size() == 1);
        CHECK(buf[0] == (char)0x61);
    }
    SECTION("2 bytes", "encode a character as a 2-byte subsequence") {
        traits_t::encode(0xf8, std::back_inserter(buf));

        REQUIRE(buf.size() == 2);
        CHECK(buf[0] == (char)0xc3);
        CHECK(buf[1] == (char)0xb8);
    }
    SECTION("3 bytes", "encode a character as a 3-byte subsequence") {
        traits_t::encode(0x20ac, std::back_inserter(buf));

        REQUIRE(buf.size() == 3);
        CHECK(buf[0] == (char)0xe2);
        CHECK(buf[1] == (char)0x82);
        CHECK(buf[2] == (char)0xac);
    }
    SECTION("4 bytes", "encode a character as a 4-byte subsequence") {
        traits_t::encode(0x1F4A9, std::back_inserter(buf));

        REQUIRE(buf.size() == 4);
        CHECK(buf[0] == (char)0xf0);
        CHECK(buf[1] == (char)0x9f);
        CHECK(buf[2] == (char)0x92);
        CHECK(buf[3] == (char)0xa9);
    }
    SECTION("returned iterator", "The return value should point just past the generated subsequence") {
        buf.resize(6);
        CHECK(traits_t::encode(0x1F4A9, buf.begin()) == buf.begin() + 4);
    }
    SECTION("encode to unsigned char", "Should be able to encode into a buffer of unsigned char") {
        std::vector<unsigned char> buf;
        traits_t::encode(0xf8, std::back_inserter(buf));

        REQUIRE(buf.size() == 2);
        CHECK(buf[0] == (unsigned char)0xc3);
        CHECK(buf[1] == (unsigned char)0xb8);
    }
    SECTION("encode to signed char", "Should be able to encode into a buffer of signed char") {
        std::vector<signed char> buf;
        traits_t::encode(0xf8, std::back_inserter(buf));

        REQUIRE(buf.size() == 2);
        CHECK(buf[0] == (signed char)0xc3);
        CHECK(buf[1] == (signed char)0xb8);
    }
}

TEST_CASE("traits/utf8/decode", "Read a UTF-8 encoded character") {
    typedef utf_traits<utf8> traits_t;

    SECTION("null", "decode a null byte") {
        unsigned char buf[] = {0x00};
        CHECK(traits_t::decode(buf) == 0x0);
    }
    SECTION("1 byte", "decode a single byte") {
        unsigned char buf[] = {0x61};
        CHECK(traits_t::decode(buf) == 0x61);
    }
    SECTION("2 bytes", "decode a 2-byte subsequence") {
        unsigned char buf[] = {0xc3, 0xb8};
        CHECK(traits_t::decode(buf) == 0xf8);
    }
    SECTION("3 bytes", "decode a 3-byte subsequence") {
        unsigned char buf[] = {0xe2, 0x82, 0xac};
        CHECK(traits_t::decode(buf) == 0x20ac);
    }
    SECTION("4 bytes", "decode a 4-byte subsequence") {
        unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
        CHECK(traits_t::decode(buf) == 0x1f4a9);
    }
    SECTION("with char", "Should be able to decode chars") {
        char buf[] = {(char)0xc3, (char)0xb8};
        CHECK(traits_t::decode(buf) == 0xf8);
    }
    SECTION("with signed char", "Should be able to decode signed chars") {
        signed char buf[] = {(signed char)0xc3, (signed char)0xb8};
        CHECK(traits_t::decode(buf) == 0xf8);
    }
}

TEST_CASE("traits/utf8/validate", "Validate a utf-8 encoded character") {
    typedef utf_traits<utf8> traits_t;

    SECTION("empty sequence is invalid", "") {
        unsigned char buf[] = {0x00};
        CHECK(!traits_t::validate(buf, buf));
    }
    SECTION("Valid null-byte", "") {
        unsigned char buf[] = {0x00};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("Valid single-byte", "") {
        unsigned char buf[] = {0x61};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("Valid 2-byte character", "") {
        unsigned char buf[] = {0xc3, 0xb8};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("Valid 3-byte character", "") {
        unsigned char buf[] = {0xe2, 0x82, 0xac};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("Valid 4-byte character", "") {
        unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }

    SECTION("Lead byte invalid", "") {
        SECTION("5-byte character sequence", "") {
            unsigned char buf[] = {0xfb, 0x9f, 0x92, 0xa9, 0x80};
            CHECK(!traits_t::validate(buf, buf+5));
        }
        SECTION("6-byte character sequence", "") {
            unsigned char buf[] = {0xfd, 0x9f, 0x92, 0xa9, 0x80, 0x80};
            CHECK(!traits_t::validate(buf, buf+6));
        }
        SECTION("Leading 0xfe byte", "") {
            unsigned char buf[] = {0xfe, 0x9f, 0x92, 0xa9, 0x80};
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        SECTION("Leading 0xff byte", "") {
            unsigned char buf[] = {0xff, 0x9f, 0x92, 0xa9, 0x80};
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        SECTION("Leading continuation byte", "") {
            unsigned char buf[] = {0x80, 0x9f, 0x92, 0xa9, 0x80};
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
    }
    SECTION("Too many bytes", "Pass in a valid 4-byte sequence, along with extra padding") {
        unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9, 0x00};
        CHECK(!traits_t::validate(buf, buf + elems(buf)));
    }

    SECTION("2-byte lead missing continuation", "") {
        unsigned char buf[] = {0xc3, 0xb8};
        CHECK(!traits_t::validate(buf, buf+1));
    }
    SECTION("3-byte lead missing continuation", "") {
        unsigned char buf[] = {0xe2, 0x82, 0xac};
        CHECK(!traits_t::validate(buf, buf+1));
        CHECK(!traits_t::validate(buf, buf+2));
    }
    SECTION("4-byte lead missing continuation", "") {
        unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
        CHECK(!traits_t::validate(buf, buf+1));
        CHECK(!traits_t::validate(buf, buf+2));
        CHECK(!traits_t::validate(buf, buf+3));
    }
    SECTION("2-byte lead with bad continuation", "") {
        unsigned char buf[] = {0xc3, 0xb8};
        buf[1] = 0x00;
        CHECK(!traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("3-byte lead with bad continuation", "") {
        {
            unsigned char buf[] = {0xe2, 0x82, 0xac};
            buf[2] = 0x00;
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        {
            unsigned char buf[] = {0xe2, 0x82, 0xac};
            buf[1] = 0x00;
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
    }
    SECTION("4-byte lead with bad continuation", "") {
        {
            unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
            buf[3] = 0x00;
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        {
            unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
            buf[2] = 0x00;
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        {
            unsigned char buf[] = {0xf0, 0x9f, 0x92, 0xa9};
            buf[1] = 0x00;
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
    }
    SECTION("overlong 2-byte sequence", "") {
        {
            unsigned char buf[] = {0xc0, 0xb8};
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
        {
            unsigned char buf[] = {0xc1, 0xb8};
            CHECK(!traits_t::validate(buf, buf + elems(buf)));
        }
    }
    SECTION("overlong 3-byte sequence", "lead byte holds all zeros") {
        unsigned char buf[] = {0xe0, 0x82, 0xac};
        CHECK(!traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("overlong 4-byte sequence", "lead byte holds all zeros, first continuation starts with 100") {
        unsigned char buf[] = {0xf0, 0x8f, 0x92, 0xa9};
        CHECK(!traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("signed char", "Validate UTF-8 data as signed chars") {
        signed char buf[] = {(char)0xc3, (char)0xb8};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
    SECTION("char", "Validate UTF-8 data as plain chars") {
        char buf[] = {(char)0xc3, (char)0xb8};
        CHECK(traits_t::validate(buf, buf + elems(buf)));
    }
}
