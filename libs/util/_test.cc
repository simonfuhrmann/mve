#include <iostream>
#include <string>
#include <cmath>
#include <csignal>

#include "refptr.h"
#include "thread.h"
#include "threadlocks.h"
#include "endian.h"
#include "fs.h"
#include "string.h"
#include "tokenizer.h"
#include "inifile.h"
#include "clocktimer.h"
#include "hrtimer.h"
#include "frametimer.h"
#include "arguments.h"

int main (void)
{
#if 0

    /* Test id parser. */
    std::vector<int> v;
    util::Arguments args;
    args.get_ids_from_string("1,2,3-5,6-6,9-7,10", v);

    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << v[i] << std::endl;

#endif

#if 0
    /* Test endian byte swapping. */
    short x(32770);

std::cout << x << std::endl;

    x = util::system::betoh(x);
    std::cout << x << std::endl;
    x = util::system::betoh(x);

    std::cout << x << std::endl;

#endif

#if 0
    /* Test to lowerstring and to upper string. */
    std::string x = "Test 1234 STRING killer cl!";
    std::cout << x << std::endl;
    std::cout << util::string::lowercase(x) << std::endl;
    std::cout << util::string::uppercase(x) << std::endl;
#endif


#if 0
    /* Backtrace. */
    signal(SIGSEGV, util::system::signal_segfault_handler);
    int* x = 0;
    *x = 10;
#endif


#if 0

    /* Newline tests. */
    std::string test = "aaa bb  cc ddddd";
    std::cout << util::string::wordwrap(test.c_str(), 10) << std::endl;

#endif


#if 0
    /* File locking test. */
    std::string filename("__test.txt");
    {
        util::fs::FileLock lock;
        bool ret = lock.acquire(filename);
        std::cout << "Ret: " << ret << std::endl;
        util::system::sleep(5000);
    }
#endif


#if 0
    /* Test FS replace extension. */
    std::string fn("myfile");
    std::cout << util::fs::replace_extension(fn, "png") << std::endl;
#endif

#if 0
    /* Test frame timer. */
    util::FrameTimer ft;
    ft.set_max_fps(10);
    while (true)
    {
        std::cout << "ft.get_time(): " << ft.get_time()
            << ", frame " << ft.get_frame_count() << std::endl;
        ft.next_frame();
    }
#endif

#if 0
    /* Test timer and sleep. */
    util::HRTimer hrtimer;
    util::ClockTimer clocktimer;

    //for (std::size_t i = 0; i < (1<<25); ++i)
    //    std::acos(-0.41243f);
    for (std::size_t i = 0; i < (1<<10); ++i)
        util::system::sleep(1);

    std::cout << "Real-time: " << hrtimer.get_elapsed() << "ms" << std::endl;
    std::cout << "CPU time: " << clocktimer.get_elapsed() << "ms" << std::endl;

#endif

#if 0
    /* Test random numbers. */
    for (int i = 0; i < 20; ++i)
    {
        std::cout << "float: " << util::system::rand_float() << "\t";
        std::cout << "int: " << util::system::rand_int() << std::endl;
    }
#endif

#if 0
    /* FS string tests. */
    std::string cwd = util::fs::get_cwd_string();
    std::cout << "CWD: " << cwd << std::endl;

    std::string fn("/data/dev/test");
    std::cout << util::fs::get_path_component(fn) << "  /  "
        << util::fs::get_file_component(fn) << std::endl;
    fn = ("dev/test");
    std::cout << util::fs::get_path_component(fn) << "  /  "
        << util::fs::get_file_component(fn) << std::endl;
    fn = ("");
    std::cout << util::fs::get_path_component(fn) << "  /  "
        << util::fs::get_file_component(fn) << std::endl;
    fn = ("test");
    std::cout << util::fs::get_path_component(fn) << "  /  "
        << util::fs::get_file_component(fn) << std::endl;
    fn = ("/");
    std::cout << util::fs::get_path_component(fn) << "  /  "
        << util::fs::get_file_component(fn) << std::endl;

#endif

#if 0
    /* Test INI reader system. */

    util::INIFile ini;
    ini.add_from_file("/tmp/testini.ini");

    std::cout << **ini.get_value("ts.test1") << std::endl;
    std::cout << **ini.get_value("ts.test2") << std::endl;

    std::cout << std::endl;
    std::cout << ini.get_value("ts.test3")->get<float>() << std::endl;
    ini.get_value("ts.test3")->set(123.34f);
    std::cout << ini.get_value("ts.test3")->get<float>() << std::endl;

    std::cout << std::endl;
    std::cout << **ini.get_value("ts.xx.test1") << std::endl;
    std::cout << **ini.get_value("ts.xx.test2") << std::endl;

    ini.to_stream(std::cout);
#endif

#if 0
    /* Test string type representations. */
    std::cout << util::string::for_type<char>() << std::endl;
    std::cout << util::string::for_type<unsigned char>() << std::endl;
    std::cout << util::string::for_type<int8_t>() << std::endl;
    std::cout << util::string::for_type<uint8_t>() << std::endl;

    std::cout << util::string::for_type<short>() << std::endl;
    std::cout << util::string::for_type<unsigned short>() << std::endl;
    std::cout << util::string::for_type<int16_t>() << std::endl;
    std::cout << util::string::for_type<uint16_t>() << std::endl;

    std::cout << util::string::for_type<int>() << std::endl;
    std::cout << util::string::for_type<unsigned int>() << std::endl;
    std::cout << util::string::for_type<int32_t>() << std::endl;
    std::cout << util::string::for_type<uint32_t>() << std::endl;

    std::cout << util::string::for_type<int64_t>() << std::endl;
    std::cout << util::string::for_type<uint64_t>() << std::endl;
    std::cout << util::string::for_type<std::size_t>() << std::endl;
    std::cout << util::string::for_type<size_t>() << std::endl;
    std::cout << util::string::for_type<ssize_t>() << std::endl;

    std::cout << util::string::for_type<float>() << std::endl;
    std::cout << util::string::for_type<double>() << std::endl;
#endif

#if 0

    /* test tokenizer. */
    util::Tokenizer t;
    t.split("This is a cool  tokenizer...");
    //print_stringvector(t);

    std::cout << "Cat: " << t.concat(4, 3) << std::endl;

    t.parse_cmd("/usr/bin/funnyapp -m123 -x 123 -s\"test string\"");
    //print_stringvector(t);

#endif

#if 0
    /* String get_filled() */
    std::cout << util::string::get_filled(0.101, 7, '.') << std::endl;

#endif

#if 0

    /* String tests: Punctate. */
    std::string test = "123456789";
    util::string::punctate(test, '.', 2);
    std::cout << "Test is now: " << test << std::endl;

    test = "12345678";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "1234567";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "123456";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "12345";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "1234";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "123";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "12";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "1";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    test = "";
    util::string::punctate(test);
    std::cout << "Test is now: " << test << std::endl;

    /* String tests: Conversion. */
    test = util::string::get(1234);
    std::cout << "Test is: " << test << std::endl;

    test = util::string::get(1234.01f);
    std::cout << "Test is: " << test << std::endl;

    test = util::string::get(1234.0001f);
    std::cout << "Test is: " << test << std::endl;

    test = util::string::get_fixed(1000.0001f, 10);
    std::cout << "Test is: " << test << std::endl;

    test = "123.1234";
    float x = util::string::convert<float>(test);
    std::cout << "Float is " << x << std::endl;

    /* String tests: Clipping. */
    test = " \t test \t    ";
    util::string::clip(test);
    std::cout << "test is " << test << std::endl;

    /* String tests: Choping. */
    test = "test\n\r\n\n";
    util::string::chop(test);
    std::cout << "test is " << test << std::endl;

    /* String tests: Normalizing. */
    test = "  string \t that\tis  pretty messy  ";
    util::string::normalize(test);
    std::cout << "test is " << test << std::endl;

#endif

#if 0

    std::string teststr = "1234567890";
    std::string left = util::string::left(teststr, 9);
    std::string right = util::string::right(teststr, 9);

    std::cout << "Left: \"" << left << "\", Right: \"" << right << "\"" << std::endl;

#endif

    return 0;
}
