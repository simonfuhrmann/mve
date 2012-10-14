// RefPtr test cases.
// Written by Simon Fuhrmann.

#include <algorithm>
#include <gtest/gtest.h>

#include "refptr.h"

using util::RefPtr;

TEST(RefPtrTest, UseCountZeroGetNull)
{
    RefPtr<int> p;
    ASSERT_EQ(p.use_count(), 0);
    ASSERT_EQ(p.get(), static_cast<int*>(0));
}

TEST(RefPtrTest, UseCountOneResetZero)
{
    RefPtr<int> p(new int(23));
    ASSERT_EQ(p.use_count(), 1);
    p.reset();
    ASSERT_EQ(p.use_count(), 0);
}

TEST(RefPtrTest, SharedCountAndReset)
{
    RefPtr<int> p1(new int(23));
    RefPtr<int> p3(p1);  // Copy constructor.
    RefPtr<int> p2 = p1;  // Implicit copy constructor.
    ASSERT_EQ(p1.use_count(), 3);
    ASSERT_EQ(p2.use_count(), 3);
    ASSERT_EQ(p3.use_count(), 3);
    p1.reset();
    ASSERT_EQ(p1.use_count(), 0);
    ASSERT_EQ(p2.use_count(), 2);
    ASSERT_EQ(p3.use_count(), 2);
    p2.reset();
    ASSERT_EQ(p1.use_count(), 0);
    ASSERT_EQ(p2.use_count(), 0);
    ASSERT_EQ(p3.use_count(), 1);
    p3.reset();
    ASSERT_EQ(p1.use_count(), 0);
    ASSERT_EQ(p2.use_count(), 0);
    ASSERT_EQ(p3.use_count(), 0);
}

TEST(RefPtrTest, UseCountN)
{
    RefPtr<int> p[10];
    p[0] = RefPtr<int>(new int(23));
    for (int i = 1; i < 10; ++i)
    {
        p[i] = p[0];  // Test assignment operator.
        ASSERT_EQ(p[i].use_count(), i+1);
    }
    for (int i = 0; i < 10; ++i)
        ASSERT_EQ(p[i].use_count(), 10);
}

TEST(RefPtrTest, DereferenceAndSwap)
{
    RefPtr<int> p1(new int(23));
    RefPtr<int> p2(new int(34));
    ASSERT_EQ(*p1, 23);
    ASSERT_EQ(*p2, 34);
    p1.swap(p2);  // Swap method.
    ASSERT_EQ(*p1, 34);
    ASSERT_EQ(*p2, 23);
    ASSERT_EQ(p1.use_count(), 1);
    ASSERT_EQ(p2.use_count(), 1);
    std::swap(p1, p2);  // Global swap overload.
    ASSERT_EQ(*p1, 23);
    ASSERT_EQ(*p2, 34);
    ASSERT_EQ(p1.use_count(), 1);
    ASSERT_EQ(p2.use_count(), 1);
}

struct TestSubject
{
    int* value;
    TestSubject (int* val) { value = val; }
    ~TestSubject (void) { *value += 1; }
    int get_value (void) { return *value; }
};

TEST(RefPtrTest, DestructionAndMemberAccess)
{
    int value = 0;
    {
        RefPtr<TestSubject> p(new TestSubject(&value));
        ASSERT_EQ(p->get_value(), 0);
        ASSERT_EQ(value, 0);
    }  // Destructor increments value.
    ASSERT_EQ(value, 1);
}

TEST(RefPtrTest, DestructionMulti)
{
    int value = 0;
    {
        RefPtr<TestSubject> p1(new TestSubject(&value));
        RefPtr<TestSubject> p2(new TestSubject(&value));
        RefPtr<TestSubject> p3(p1);
        ASSERT_EQ(value, 0);
    }
    ASSERT_EQ(value, 2);
}

TEST(RefPtrTest, AssignmentAndGet)
{
    int* ptr = new int(23);
    RefPtr<int> p1(ptr);
    ASSERT_EQ(p1.get(), ptr);
    ASSERT_EQ(p1.use_count(), 1);
    p1 = p1;  // Assignment to self.
    ASSERT_EQ(p1.get(), ptr);
    ASSERT_EQ(p1.use_count(), 1);

    RefPtr<int> p2 = p1;  // Implicit copy constructor.
    ASSERT_EQ(p2.get(), ptr);
    ASSERT_EQ(p2.use_count(), 2);

    p2 = p1;  // Assignment operator.
    ASSERT_EQ(p2.get(), ptr);
    ASSERT_EQ(p2.use_count(), 2);
}

TEST(RefPtrTest, ComparisonSameType)
{
    int* ptr1 = new int(123);
    int* ptr2 = new int(234);
    RefPtr<int> p1(ptr1);
    RefPtr<int> p2(ptr2);
    ASSERT_TRUE(p1 == ptr1);
    ASSERT_TRUE(p2 == ptr2);
    if (ptr1 < ptr2)
        ASSERT_TRUE(p1 < p2);
    else
        ASSERT_TRUE(p2 < p1);
    p2 = p1;
    ASSERT_EQ(p1.get(), p2.get());
    ASSERT_TRUE(p1 == p2);
    ASSERT_FALSE(p1 != p2);
}

struct Base
{
    virtual int get_virtual (void) { return 10; }
    int get (void) { return 20; }
};

struct Derived : public Base
{
    virtual int get_virtual (void) { return 30; }
    int get (void) { return 40; }
};

TEST(RefPtrTest, AssignmentDifferentType)
{
    RefPtr<Base> p1(new Base);
    RefPtr<Derived> p2(new Derived);

    ASSERT_EQ(p1->get_virtual(), 10);
    ASSERT_EQ(p1->get(), 20);
    ASSERT_EQ(p2->get_virtual(), 30);
    ASSERT_EQ(p2->get(), 40);
    p1 = p2;  // Assignment different type.
    ASSERT_EQ(p1->get_virtual(), 30);
    ASSERT_EQ(p1->get(), 20);
}

TEST(RefPtrTest, ComparisonDifferentType)
{
    Base* baseptr = new Base();
    Derived* derivedptr = new Derived();
    RefPtr<Derived> p1(derivedptr);
    RefPtr<Base> p2(p1);  // Explicit copy constructor from other type.
    RefPtr<Base> p3(baseptr);
    RefPtr<Base> p4 = p1;  // Implicit copy constructor from other type.

    ASSERT_TRUE(p1 == p2);
    ASSERT_FALSE(p1 != p2);
    ASSERT_TRUE(p1 != p3);
    ASSERT_FALSE(p1 == p3);

    ASSERT_TRUE(p2 == p1);
    ASSERT_FALSE(p2 != p1);
    ASSERT_TRUE(p3 != p1);
    ASSERT_FALSE(p3 == p1);

    if (baseptr < derivedptr)
        ASSERT_TRUE(p3 < p1);
    else
        ASSERT_TRUE(p1 < p3);
}
