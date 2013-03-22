#include <vector>
#include <iostream>
#include <iterator>
#include <string>
#include "utf.hpp"

int main() {
    const char str[] = "hello world";
    utf::stringview<utf::utf8> sv(str, str + sizeof(str));
    std::cout << "number of code units: " << sv.codeunits() << '\n';
    std::cout << "byte length: " << sv.bytes() << '\n';
    std::cout << "byte length as utf16: " << sv.bytes<utf::utf16>() << '\n';
    std::vector<char16_t> buf(sv.codeunits<utf::utf16>(), 0);
    sv.to<utf::utf16>(buf.begin());
    std::cout << "utf16 code units: ";
    for (std::vector<char16_t>::iterator c = buf.begin(); c != buf.end(); ++c) {
        std::cout << std::hex << (uint16_t)*c << ' ';
    }
    std::cout << '\n';
    
    std::string s = "hello world";

    utf::make_stringview(s.begin(), s.end());

    const wchar_t* str2 = L"hell\xf8 world";
    auto sv2 = utf::make_stringview(str2, str2 + 11);
    std::string v;
    sv2.to<utf::utf8>(std::back_inserter(v));
    std::cout << v;
}
