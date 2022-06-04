add_rules("mode.debug", "mode.release")

set_languages("c++20")
add_requires("fmt")
add_requires("gtest 1.11.0")

target("0327")
    add_files("main.cpp")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/bplustree/*.cpp")
    add_packages("fmt")
    add_includedirs("include")

target("page_test")
    add_files("tests/page_test.cpp")
    add_files("src/*.cpp")
    add_files("src/bplustree/*.cpp")
    add_packages("fmt", "gtest")
    add_includedirs("include")

