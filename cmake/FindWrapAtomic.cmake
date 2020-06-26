# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapAtomic::WrapAtomic)
    set(WrapAtomic_FOUND ON)
    return()
endif()

include(CheckCXXSourceCompiles)

set (atomic_test_sources "#include <atomic>
#include <cstdint>

void test(volatile std::atomic<std::int64_t> &a)
{
    std::int64_t v = a.load(std::memory_order_acquire);
    while (!a.compare_exchange_strong(v, v + 1,
                                      std::memory_order_acq_rel,
                                      std::memory_order_acquire)) {
        v = a.exchange(v - 1);
    }
    a.store(v + 1, std::memory_order_release);
}

int main(int, char **)
{
    void *ptr = (void*)0xffffffc0; // any random pointer
    test(*reinterpret_cast<std::atomic<std::int64_t> *>(ptr));
    return 0;
}")

check_cxx_source_compiles("${atomic_test_sources}" HAVE_STDATOMIC)
if(NOT HAVE_STDATOMIC)
    set(_req_libraries "${CMAKE_REQUIRE_LIBRARIES}")
    set(CMAKE_REQUIRE_LIBRARIES "atomic")
    check_cxx_source_compiles("${atomic_test_sources}" HAVE_STDATOMIC_WITH_LIB)
    set(CMAKE_REQUIRE_LIBRARIES "${_req_libraries}")
endif()

add_library(WrapAtomic::WrapAtomic INTERFACE IMPORTED)
if(HAVE_STDATOMIC_WITH_LIB)
    target_link_libraries(WrapAtomic::WrapAtomic INTERFACE atomic)
endif()

set(WrapAtomic_FOUND 1)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapAtomic DEFAULT_MSG WrapAtomic_FOUND)
