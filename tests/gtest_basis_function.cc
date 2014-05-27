// Test cases for basis function.
// Written by Simon Fuhrmann.

#include <fstream> //tmp

#include <gtest/gtest.h>

#include "util/timer.h"
#include "fssr/basis_function.h"

TEST(BasisFunctionTest, TestWeightingFunction)
{
    for (double x = -3.0; x < 3.0; x += 0.1)
        EXPECT_GE(fssr::weighting_function_x(x), 0.0);
    EXPECT_EQ(0.0, fssr::weighting_function_x(-3.0));
    EXPECT_EQ(0.0, fssr::weighting_function_x(3.0));

    for (double y = -3.0; y < 3.0; y += 0.3)
        for (double x = -3.0; x < 3.0; x += 0.3)
        {
            EXPECT_EQ(fssr::weighting_function_yz(0.0, x),
                fssr::weighting_function_yz(x, 0.0));
            EXPECT_EQ(fssr::weighting_function_yz(y, x),
                fssr::weighting_function_yz(x, y));
        }
}

TEST(BasisFunctionTest, TestMPUWeightingFunction)
{
    std::ofstream out("/tmp/data.txt");
    for (double x = -3.5; x <= 3.5; x += 0.1)
        out << x << " " << fssr::weighting_function_mpu(x) << std::endl;
    out.close();
}
