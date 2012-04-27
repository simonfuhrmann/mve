#include <iostream>
#include <cassert>

#include "vector.h"
#include "jacobisolver.h"
#include "matrix.h"
#include "matrixtools.h"
#include "algo.h"
#include "permute.h"
#include "bspline.h"

void vector_tests (void)
{
    using namespace math;
    assert(Vec2f(1.0f) == Vec2f(1.0f, 1.0f));
    assert(Vec2f(1.0f, 2.0f) + Vec2f(2.0f, 3.0f) == Vec2f(3.0f, 5.0f));
    assert(Vec2f(1.0f, 2.0f) - Vec2f(2.0f, 3.0f) == Vec2f(-1.0f, -1.0f));
    assert(Vec2f(10.0f, 0.0f).normalize() == Vec2f(1.0f, 0.0f));
    assert(Vec2f(3.0f, 4.0f).norm() == 5.0f);
    assert(Vec2f(2.0f, 2.0f).square_norm() == 8.0f);
    assert(Vec2f().dim() == 2);
    assert(Vec3f().dim() == 3);
    assert(Vec4f().dim() == 4);
    assert(Vec2f(-1.0f, 2.0f).abs_value() == Vec2f(1.0f, 2.0f));
    assert(Vec2f(-1.0f, -2.0f).abs_value() == Vec2f(1.0f, 2.0f));
    assert(Vec2f(1.0f, -2.0f).abs_value() == Vec2f(1.0f, 2.0f));
    assert(Vec2f(1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)) == 42.0f);
    assert(Vec2f(1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)) == 42.0f);
    assert(Vec2f(-1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)) == 38.0f);
    assert(Vec2f(1.0f, -4.0f).dot(Vec2f(2.0f, 10.0f)) == -38.0f);
    assert(Vec2f(-1.0f, -4.0f).dot(Vec2f(2.0f, 10.0f)) == -42.0f);
    assert(Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.1f));
    assert(Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.2f));
    assert(!Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.05f));
    assert(Vec2f(0.0f, 0.0f).is_similar(Vec2f(0.0f, 0.0f), 0.0f));
    assert(Vec2f(1.0f, 0.0f).is_similar(Vec2f(1.0f, 0.0f), 0.0f));
    assert(Vec2f(1.0f, 100.0f).maximum() == 100.0f);
    assert(Vec2f(1.0f, 100.0f).minimum() == 1.0f);
    assert(Vec2f(-1000.0f, 100.0f).minimum() == -1000.0f);
    assert(Vec2f(-1000.0f, 100.0f).maximum() == 100.0f);
    assert(Vec3f(2.0f, 2.0f, 2.0f).product() == 8.0f);
    assert(Vec3f(2.0f, 2.0f, 2.0f).sum() == 6.0f);
    assert(Vec3f(1.0f, 2.0f, 3.0f).sort_asc() == Vec3f(1.0f, 2.0f, 3.0f));
    assert(Vec3f(3.0f, 2.0f, 1.0f).sort_asc() == Vec3f(1.0f, 2.0f, 3.0f));
    assert(Vec3f(1.0f, 3.0f, 2.0f).sort_asc() == Vec3f(1.0f, 2.0f, 3.0f));
    assert(Vec3f(1.0f, 2.0f, 3.0f).sort_desc() == Vec3f(3.0f, 2.0f, 1.0f));
    assert(Vec3f(3.0f, 2.0f, 1.0f).sort_desc() == Vec3f(3.0f, 2.0f, 1.0f));
    assert(Vec3f(1.0f, 3.0f, 2.0f).sort_desc() == Vec3f(3.0f, 2.0f, 1.0f));
    assert(Vec3f(1.0f, 0.0f, 0.0f).cross(Vec3f(0.0f, 1.0f, 0.0f)) == Vec3f(0.0f, 0.0f, 1.0f));
    //assert(Vec2f(1.0f, 0.0f).cross(Vec2f(0.0f, 1.0f)) == Vec2f(0.0f, 1.0f));

    assert(Vec2f(1.0f, 2.0f).cw_mult(Vec2f(5.0f, 6.0f)) == Vec2f(5.0f, 12.0f));
    assert(Vec2f(3.0f, 2.0f).cw_mult(Vec2f(2.0f, 3.0f)) == Vec2f(6.0f, 6.0f));
}

void matrix_tests (void)
{
    using namespace math;
    Matrix3f test(999.0f);
    test(0,0) = 1.0f; test(0,1) = 2.0f; test(0,2) = 3.0f;
    test(1,0) = 4.0f; test(1,1) = 5.0f; test(1,2) = 6.0f;
    test(2,0) = 7.0f; test(2,1) = 8.0f; test(2,2) = 9.0f;

    Matrix<float,3,2> M1;
    M1(0,0) = 1.0f; M1(0,1) = 2.0f;
    M1(1,0) = 3.0f; M1(1,1) = 4.0f;
    M1(2,0) = 5.0f; M1(2,1) = 6.0f;

    Matrix<float,2,3> M2;
    M2(0,0) = 5.0f; M2(0,1) = 6.0f; M2(0,2) = 1.0f;
    M2(1,0) = 1.0f; M2(1,1) = 2.0f; M2(1,2) = 3.0f;

    /* Matrix matrix multiplication. */
    Matrix3f R1 = M1.mult(M2);
    assert(R1(0,0) == 7.0f); assert(R1(0,1) == 10.0f);assert(R1(0,2) == 7.0f);
    assert(R1(1,0) == 19.0f);assert(R1(1,1) == 26.0f);assert(R1(1,2) == 15.0f);
    assert(R1(2,0) == 31.0f);assert(R1(2,1) == 42.0f);assert(R1(2,2) == 23.0f);

    /* Matrix matrix multiplication. */
    Matrix2f R2 = M2.mult(M1);
    assert(R2(0,0) == 28.0f); assert(R2(0,1) == 40.0f);
    assert(R2(1,0) == 22.0f); assert(R2(1,1) == 28.0f);

    /* Matrix matrix substraction. */
    Matrix3f ones(1.0f);
    assert((test - ones)(0,0) == 0.0f);
    assert((test - ones)(0,1) == 1.0f);
    assert((test - ones)(0,2) == 2.0f);

    /* Matrix access, min, max, square check, . */
    assert(test.col(1) == Vec3f(2.0f, 5.0f, 8.0f));
    assert(test.row(1) == Vec3f(4.0f, 5.0f, 6.0f));
    assert(Matrix3f(1.0f).minimum() == 1.0f);
    assert(Matrix3f(1.0f).maximum() == 1.0f);
    assert((Matrix<float,3,3>()).is_square());
    assert(!(Matrix<float,3,4>()).is_square());
    assert(test(1,2) == 6.0f);
    assert(test.transposed()(1,2) == 8.0f);

    assert(!test.is_similar(ones, 0.0f));
    assert(!test.is_similar(ones, 5.0f));
    assert(test.is_similar(ones, 8.0f));

    assert(test.mult(Vec3f(1.0f, 2.0f, 3.0f)) == Vec3f(14.0f, 32.0f, 50.0f));

#if 0
    Matrix4d A(0.0);
    A(0,0)=-5.56345;   A(0,1)=-5.43511;   A(0,2)=-5.43511;   A(0,3)=-5.43511;
    A(1,0)=24.2440;   A(1,1)=24.3723;   A(1,2)=24.3723;   A(1,3)=24.2440;
    A(2,0)=15.1236;   A(2,1)=15.2519;   A(2,2)=15.1236;   A(2,3)=15.1236;
    A(3,0)=1.0000;    A(3,1)=1.0000;    A(3,2)=1.0000;    A(3,3)=1.0000;

    // A(0,0) = 56.0f; A(0,1) = 16.0f; A(0,2) = 16.0f; A(0,3) = 68.0f;
    // A(1,0) = 46.0f; A(1,1) = 79.0f; A(1,2) = 60.0f; A(1,3) = 74.0f;
    // A(2,0) = 1.0f; A(2,1) = 31.0f; A(2,2) = 26.0f; A(2,3) = 45.0f;
    // A(3,0) = 33.0f; A(3,1) = 52.0f; A(3,2) = 65.0f; A(3,3) = 8.0f;
    std::cout << A << std::endl;
    // std::cout << "Det: " << math::matrix_determinant(A) << std::endl;
    std::cout << "Inv: " << math::matrix_inverse(A) << std::endl;
#endif
}

void
gaussian_tests (void)
{
    using namespace math;

    assert(algo::gaussian(0.0f, 1.0f) == 1.0f);

    assert(MATH_EPSILON_EQ(algo::gaussian(1.0f, 1.0f),
        0.606530659712633f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(-1.0f, 1.0f),
        0.606530659712633f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(2.0f, 1.0f),
        0.135335283236613f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(-2.0f, 1.0f),
        0.135335283236613f, 0.00000000000001f));

    assert(MATH_EPSILON_EQ(algo::gaussian(1.0f, 2.0f),
        0.882496902584595f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(-1.0f, 2.0f),
        0.882496902584595f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(2.0f, 2.0f),
        0.606530659712633f, 0.00000000000001f));
    assert(MATH_EPSILON_EQ(algo::gaussian(-2.0f, 2.0f),
        0.606530659712633f, 0.00000000000001f));
}

void
permutation_tests (void)
{
    /* Test permutations. */
    std::vector<float> v;
    v.push_back(0.5f);
    v.push_back(1.5f);
    v.push_back(2.5f);
    v.push_back(3.5f);
    v.push_back(4.5f);
    v.push_back(5.5f);

    std::vector<float> bak(v);

    std::vector<std::size_t> p;
    p.push_back(0); // 0
    p.push_back(4); // 1
    p.push_back(5); // 2
    p.push_back(2); // 3
    p.push_back(1); // 4
    p.push_back(3); // 5

    math::algo::permute_reloc<float, std::size_t>(v, p);

    for (std::size_t i = 0; i < p.size(); ++i)
        assert(v[p[i]] == bak[i]);

    v = bak;
    math::algo::permute_math<float, std::size_t>(v, p);

    for (std::size_t i = 0; i < p.size(); ++i)
        assert(v[i] == bak[p[i]]);
}

void
vector_tools_tests (void)
{
    std::vector<int> vec;
    vec.push_back(99);
    vec.push_back(98);
    vec.push_back(0);
    vec.push_back(97);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(96);
    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(95);
    vec.push_back(94);

    std::vector<bool> dlist;
    dlist.push_back(true);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(true);

    math::algo::vector_clean(vec, dlist);

    for (int i = 0; i < (int)vec.size(); ++i)
        assert(vec[i] == i);

    vec.clear();
    dlist.clear();
    math::algo::vector_clean(vec, dlist);

    vec.push_back(1);
    dlist.push_back(true);
    math::algo::vector_clean(vec, dlist);

    vec.clear();
    dlist.clear();
    vec.push_back(1);
    dlist.push_back(false);
    math::algo::vector_clean(vec, dlist);
}

void
algorithm_tests (void)
{
    {
        float f[5] = { 1.0f, 0.5f, 0.0f, 0.2f, 0.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        assert(id == 2);
        id = math::algo::max_element_id(f, f+5);
        assert(id == 0);
    }

    {
        float f[5] = { -1.0f, 0.5f, 0.0f, 0.2f, 1.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        assert(id == 0);
        id = math::algo::max_element_id(f, f+5);
        assert(id == 4);
    }

    {
        float f[5] = { 1.0f, 0.5f, 1.1f, 0.2f, -0.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        assert(id == 4);
        id = math::algo::max_element_id(f, f+5);
        assert(id == 2);
    }
}

void
solver_tests (void)
{
    using namespace math;
    JacobiSolverParams<float> params;
    params.maxIter = 100;
    params.minResidual = 0.0f;

    Matrix3f A;
    A(0,0) = -2.0f; A(0,1) = 0.0f; A(0,2) = 0.0f;
    A(1,0) = 4.0f; A(1,1) = -3.0f; A(1,2) = -1.0f;
    A(2,0) = 0.0f; A(2,1) = -4.0f; A(2,2) = 4.0f;

    Vec3f rhs(2.0f, 4.0f, 16.0f);
    Vec3f exactSolution(-1.0f, -3.0f, 1.0f);

    JacobiSolver<float, 3> solver(A, params);
    Vec3f solution = solver.solve(rhs, Vec3f(0.0f));

    //std::cout << "iterations: " << solver.getInfo().maxIter << ", residual norm: " << solver.getInfo().minResidual << std::endl;
    //std::cout << "Solution: " << solution << std::endl;
    assert(exactSolution.is_similar(solution, 0.0f));
}


#include <fstream>
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"

void
bspline_tests (void)
{
    using namespace math;

#if 0
    BSpline<math::Vec3f, float> s;
    s.set_degree(2);
    s.add_point(math::Vec3f(0,0,0));
    s.add_point(math::Vec3f(1,1,0));
    s.add_point(math::Vec3f(2,2,0));
    s.uniform_knots(0.0f, 1.0f);

    std::ofstream out("/tmp/spline.off");
    out << "OFF\n1000 0 0" << std::endl;
    for (int i = 0; i < 101; ++i)
    {
        float t = (float)i / 100.0f;
        math::Vec3f p = s.evaluate(t);
        out << p << std::endl;
        std::cout << "t=" << t << ": " <<  p << std::endl;
    }
    out.close();
#endif

    BSpline<math::Vec3f, float> s;
    s.set_degree(3);
    s.add_point(math::Vec3f(0,0,0));
    s.add_point(math::Vec3f(1,1,0));
    s.add_point(math::Vec3f(2,2,0));
    s.add_point(math::Vec3f(3,3,0));
    s.add_point(math::Vec3f(4,4,0));
    s.add_point(math::Vec3f(5,5,0));
    s.add_point(math::Vec3f(6,6,0));

    // Need 11 knots
    //for (int i = 0; i < 11; ++i)
    //    s.add_knot((float)i);

    s.add_knot(0);
    s.add_knot(0);
    s.add_knot(0);
    s.add_knot(1);
    s.add_knot(2);
    s.add_knot(3);
    s.add_knot(4);
    s.add_knot(5);
    s.add_knot(5);
    s.add_knot(5);
    s.add_knot(6);

    s.scale_knots(0.0f, 1.0f);
    for (std::size_t i = 0; i < s.get_knots().size(); ++i)
        std::cout << "Knot " << i << ": " << s.get_knots()[i] << std::endl;


#if 0
    BSpline<math::Vec3f, float> s;
    s.set_degree(3);
    s.reserve(5);

    std::ofstream out("/tmp/spline.ply");
    out << "ply" << std::endl << "format ascii 1.0" << std::endl;
    out << "element vertex 1050" << std::endl;
    out << "property float x" << std::endl;
    out << "property float y" << std::endl;
    out << "property float z" << std::endl;
    out << "property uchar r" << std::endl;
    out << "property uchar g" << std::endl;
    out << "property uchar b" << std::endl;
    out << "end_header" << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        math::Vec3f p[4] = { math::Vec3f(-1,-1,0+i), math::Vec3f(1,-1,0+i), math::Vec3f(1,1,0+i), math::Vec3f(-1,1,0+i) };
        for (int j = 0; j < 4; ++j)
        {
            out << p[j] << " 255 0 0" << std::endl;
            s.add_point(p[j]);
        }
    }
    for (int i = 0; i < 10; ++i)
    {
        math::Vec3f p(0,0,9-i);
        out << p << " 255 0 0" << std::endl;
        s.add_point(p);
    }

    float min = 0.0f;
    float max = 10.0f;
    s.uniform_knots(min, max);
    for (std::size_t i = 0; i < 1000; ++i)
    {
        float t = min + (float)i * (max - min) / 999.0f;
        math::Vec3f p = s.evaluate(t);
        out << p << " 0 255 0" << std::endl;
    }
    out.close();
#endif
}

#include "octreetools.h"

void
ray_ray_intersect_test()
{
    math::Vec3d p1, d1, p2,d2;

    p1[0] = 0; p1[1] = 0; p1[2] = 0;
    d1[0] = 0; d1[1] = 0; d1[2] = 1;

    p2[0] = 0; p2[1] = 0; p2[2] = 0;
    d2[0] = 0; d2[1] = 1; d2[2] = 0;
    
    math::Vec2d t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);

    assert(math::Vec2d(0.0) == t);

    p2[0] = 1.0;
    t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);
    assert(math::Vec2d(0.0) == t);

    p2[0] = 1; p2[1] = 1; p2[2] = 1;
    t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);
    assert((t[0]*d1+p1) == math::Vec3f(0,0,1));
    assert((t[1]*d2+p2) == math::Vec3f(1,0,1));


}

#include "util/system.h"

int main (void)
{
    vector_tests();
    matrix_tests();
    gaussian_tests();
    permutation_tests();
    vector_tools_tests();
    algorithm_tests();
    solver_tests();
    bspline_tests();
    ray_ray_intersect_test();

    return 0;
}
