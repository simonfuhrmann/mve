#include <iostream>
#include <string>
#include <cmath>
#include <csignal>

#include "refptr.h"
#include "thread.h"
#include "threadlocks.h"
#include "endian.h"
#include "filesystem.h"
#include "string.h"
#include "tokenizer.h"
#include "timer.h"
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
    /* Backtrace. */
    signal(SIGSEGV, util::system::signal_segfault_handler);
    int* x = 0;
    *x = 10;
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
    util::WallTimer walltimer;
    util::ClockTimer clocktimer;

    //for (std::size_t i = 0; i < (1<<25); ++i)
    //    std::acos(-0.41243f);
    for (std::size_t i = 0; i < (1<<10); ++i)
        util::system::sleep(1);

    std::cout << "Real-time: " << walltimer.get_elapsed() << "ms" << std::endl;
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

    return 0;
}
