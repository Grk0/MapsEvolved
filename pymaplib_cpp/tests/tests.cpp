// Copyright 2015 Christian Aichinger <Greek0@gmx.net>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE Master Test Suite
#include <boost/test/unit_test.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/results_reporter.hpp>
#include <boost/test/debug.hpp>
#include <boost/algorithm/string.hpp>

#include "../include/util.h"


TestConfig testconfig;

void TestConfig::initialize(int argc, char* argv[]) {
    m_progname = argv[0];
    add(argv + 1, argv + argc);
}
bool TestConfig::want_test_list() const {
    return m_args.size() == 1 &&
            (m_args.find("-l") != m_args.cend() ||
            m_args.find("--list") != m_args.cend());
}
boost::optional<unsigned int> TestConfig::concurrency_test_msecs() const {
    auto cname = std::string("--concurrency-secs=");
    auto it = std::find_if(m_args.cbegin(), m_args.cend(),
        [cname](const std::string &s) -> bool {
            return boost::starts_with(s, cname);
    });
    if (it == m_args.cend()) {
        return boost::optional<unsigned int>();
    }
    auto arg = boost::erase_first_copy(*it, cname);
    try {
        // Don't worry about rounding semantics, 1ms more or less won't matter.
        auto ms = static_cast<unsigned int>(std::stof(arg) * 1000);
        return boost::make_optional<unsigned int>(ms);
    } catch(const std::logic_error &err) {
        std::cerr << "Could not parse argument '" << *it << "':" << std::endl;
        std::cerr << "  " << err.what() << std::endl;
        return boost::optional<unsigned int>();
    }
};


struct test_tree_reporter : boost::unit_test::test_tree_visitor {
public:
    test_tree_reporter(std::ostream &stream)
        : m_indent(-4),      // Don't output the master_test_suite name.
          m_stream(stream)
    {}

private:
    virtual void visit(boost::unit_test::test_case const& tc) {
        m_stream << std::setw(m_indent) << "" << tc.p_name << "\n";
    }
    virtual bool test_suite_start(boost::unit_test::test_suite const& ts) {
        if( m_indent >= 0 ) {
            m_stream << std::setw(m_indent) << "" << ts.p_name << "\n";
        }
        m_indent += 4;
        return true;
    }
    virtual void test_suite_finish(boost::unit_test::test_suite const&) {
        m_indent -= 4;
    }

    int m_indent;
    std::ostream &m_stream;
};


static bool init_maplib_tests() {
    if (!init_unit_test()) {
        return false;
    }
    if (testconfig.want_test_list()) {
        auto& stream = boost::unit_test::results_reporter::get_stream();
        stream << "Available test suites and cases:" << std::endl;

        auto p_id = boost::unit_test::framework::master_test_suite().p_id;
        test_tree_reporter content_reporter(stream);
        traverse_test_tree(p_id, content_reporter);

        // Disable memory leak detection when only showing the test case list.
        // Otherwise an ugly error is printed due to the std::exit() call.
        boost::debug::detect_memory_leaks(false);

        // Exit directly to suppress running tests or showing error messages.
        std::exit(0);
    }
    return true;
}


#ifdef BOOST_TEST_NO_MAIN
int main(int argc, char* argv[]) {
    testconfig.initialize(argc, argv);
    int ret = boost::unit_test::unit_test_main(&init_maplib_tests, argc, argv);
    return ret;  // Conditional breakpoint here.
}
#endif
