#ifndef NP_UTF_HPP
#define NP_UTF_HPP

namespace utf {
    struct utf8;

    typedef char32_t codepoint_type;

    template <typename E>
    struct utf_traits;

    template <>
    struct utf_traits<utf8> {
        typedef char codeunit_type;
        static size_t read_length(codeunit_type c) { return size_t(); }
        static size_t write_length(codepoint_type c) {return size_t(); }

        // responsible only for validating the utf8 encoded subsequence, not the codepoint it maps to
        template <typename T>
        static bool validate(const T* first, const T* last) { return bool(); }

        template <typename OutIt>
        static OutIt encode(codepoint_type c, OutIt dest) { return dest; }

        template <typename T>
        static codepoint_type decode(const T* c) { return codepoint_type(); }
    };
}
#endif
