#!/usr/bin/env bash
# SpliX C++23 Capability Checker
# Checks for key C++23/20 features required by the modernized codebase.

echo "Checking C++23 Feature Support..."

cat <<EOF > cpp23_test.cpp
#include <version>
#include <span>
#include <string_view>
#include <expected>
#include <semaphore>

int main() {
#if __cplusplus >= 202302L
    return 0;
#else
    return 1;
#endif
}
EOF

# Try to compile with the detected compiler
if c++ -std=c++23 cpp23_test.cpp -o cpp23_test 2>/dev/null; then
    echo "✅ Success: C++23 is supported by your compiler."
    rm cpp23_test.cpp cpp23_test
    exit 0
elif c++ -std=c++2b cpp23_test.cpp -o cpp23_test 2>/dev/null; then
    echo "✅ Success: C++23 (experimental) is supported via -std=c++2b."
    rm cpp23_test.cpp cpp23_test
    exit 0
else
    echo "❌ Error: C++23 is NOT supported by '$(c++ --version | head -n 1)'."
    echo "Modernized SpliX requires GCC 12+, Clang 15+, or MSVC 19.33+."
    rm cpp23_test.cpp
    exit 1
fi
