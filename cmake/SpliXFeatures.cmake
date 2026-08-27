include(CheckCXXSourceCompiles)

function(splix_require_cxx23)
    if(NOT DEFINED SPLIX_REQUIRED_CXX_STANDARD)
        set(SPLIX_REQUIRED_CXX_STANDARD 23)
    endif()

    set(_saved_standard "${CMAKE_CXX_STANDARD}")
    set(_saved_required "${CMAKE_CXX_STANDARD_REQUIRED}")
    set(CMAKE_CXX_STANDARD "${SPLIX_REQUIRED_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    set(_probe_variable
        "SPLIX_HAS_REQUIRED_CXX${SPLIX_REQUIRED_CXX_STANDARD}")
    check_cxx_source_compiles([=[
        #include <expected>
        #include <semaphore>
        #include <span>
        #include <thread>

        int main()
        {
            std::expected<int, int> value{1};
            std::counting_semaphore<1> ready{1};
            const int item = value.value();
            std::span<const int> view{&item, 1};
            std::jthread worker([] {});
            ready.acquire();
            return view.front() == 1 ? 0 : 1;
        }
    ]=] ${_probe_variable})

    set(CMAKE_CXX_STANDARD "${_saved_standard}")
    set(CMAKE_CXX_STANDARD_REQUIRED "${_saved_required}")

    if(NOT "${${_probe_variable}}")
        message(FATAL_ERROR
            "SpliX requires C++23 library support for std::expected")
    endif()

    set(SPLIX_HAS_REQUIRED_CXX23 TRUE PARENT_SCOPE)
endfunction()
