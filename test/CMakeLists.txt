project(ut_heap)

set(UT_SOURCES
    ut_bitmap.cpp
)

mark3_add_executable(ut_bitmap ${UT_SOURCES})

target_link_libraries(ut_bitmap.elf
    ut_base
    mark3
    mark3c
    memutil
    heap
)

set(UT_SOURCES
    ut_slab.cpp
)

mark3_add_executable(ut_slab ${UT_SOURCES})

target_link_libraries(ut_slab.elf
    ut_base
    mark3
    mark3c
    memutil
    heap
)

set(UT_SOURCES
    ut_fixedblock.cpp
)

mark3_add_executable(ut_fixedblock ${UT_SOURCES})

target_link_libraries(ut_fixedblock.elf
    ut_base
    mark3
    mark3c
    memutil
    heap
)

set(UT_SOURCES
    ut_arena.cpp
)

mark3_add_executable(ut_arena ${UT_SOURCES})

target_link_libraries(ut_arena.elf
    ut_base
    mark3
    mark3c
    memutil
    heap
)
