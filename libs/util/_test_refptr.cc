#include <iostream>
#include <vector>
#include <string>

#include "thread.h"
#include "refptr.h"
#include "clocktimer.h"

class RefTestClass;
typedef util::RefPtr<RefTestClass> Ptr;
class RefTestClass
{
public:
    RefTestClass (void) { std::cout << "Ctor " << this << std::endl; }
    ~RefTestClass (void) { std::cout << "Dtor " << this << std::endl; }
    void squeeze (void) { /*std::cout << "Ouch " << this << std::endl;*/ }
};

class RefStressTest : public util::Thread
{
private:
    Ptr test;

public:
    RefStressTest (Ptr ptrtest) : test(ptrtest) {}
    void* run (void)
    {
        for (int i = 0; i < 1000000; ++i)
        {
            Ptr xtest = test;
            xtest->squeeze();
        }
        delete this;
        std::cout << "Done" << std::endl;
    }

};

void
print_stringvector (std::vector<std::string> const& strvec)
{
    for (std::size_t i = 0; i < strvec.size(); ++i)
        std::cout << i << ": " << strvec[i] << std::endl;
}


int main (void)
{
#if 0

    /* Smart pointer swap test. */
    util::RefPtr<int> myint1(new int(1));
    util::RefPtr<int> myint2(new int(2));

    util::ClockTimer timer;
    for (std::size_t i = 0; i < 100000000; ++i)
        myint1.swap(myint2);
    std::cout << "Took " << timer.get_elapsed() << "ms." << std::endl;
    timer.reset();
    for (std::size_t i = 0; i < 100000000; ++i)
        std::swap(myint1, myint2);
    std::cout << "Took " << timer.get_elapsed() << "ms." << std::endl;

#endif

#if 0
    /* Smart pointer stress test. */

    Ptr test(new RefTestClass);
    for (int i = 0; i < 10; ++i)
    {
        RefStressTest* thread = new RefStressTest(test);
        thread->pt_create();
    }

    ::sleep(3);
    std::cout << "Use count: " << test.use_count() << std::endl;
#endif

#if 0

    /* RefPtr tests. */
    {
        Ptr i(new RefTestClass);
        std::cout << "Scope ending..." << std::endl;
    }
    std::cout << "-------" << std::endl;
    {
        Ptr i(new RefTestClass);
        Ptr j(new RefTestClass);
        std::cout << "Scope ending..." << std::endl;
    }
    std::cout << "-------" << std::endl;
    {
        Ptr i(new RefTestClass);
        Ptr j(i);
        i->squeeze();
        j->squeeze();
        std::cout << "use_count: " << j.use_count() << std::endl;
        j.reset();
        std::cout << "use_count: " << j.use_count() << std::endl;
        std::cout << "use_count: " << i.use_count() << std::endl;
        std::cout << "Scope ending..." << std::endl;
    }
    std::cout << "-------" << std::endl;
#endif

    return 0;
}
