#include <eigen3/Eigen/Sparse>

#include <iostream>

#include "util/timer.h"
#include "sfm/ba_sparse_matrix.h"
#include "sfm/ba_dense_vector.h"

typedef sfm::ba::SparseMatrix<double> SparseMatrix;
typedef Eigen::SparseMatrix<double> EigenMatrix;
typedef std::vector<Eigen::Triplet<double> > EigenTriplets;

#define MATRIX_ROWS 10000
#define MATRIX_COLS 10000
#define NON_ZERO_ENTRIES 1000000

/*
 * Timings with 1000x1000 and 1M elements:
 *  SfM Tiplets 1: 32
 *  SfM Tiplets 2: 87
 *  SfM RM CM Multiply: 73105
 *  SfM CM CM Multiply: 1102
 *  SfM Transpose: 0
 *  SfM Transpose 2:
 *  Eig Tiplets 1: 33
 *  Eig Tiplets 2: 32
 *  Eig Multiply: 3022
 *  Eig Transpose: 13
 *
 * Timings with 1000x1000 and 100k elements:
 *   SfM Tiplets 1: 8
 *   SfM Tiplets 2: 7
 *   SfM Multiply: 7086
 *   SfM RM CM Multiply: 7102
 *   SfM CM CM Multiply: 119
 *   SfM Transpose: 0
 *   Eig Tiplets 1: 4
 *   Eig Tiplets 2: 3
 *   Eig Multiply: 45
 *   Eig Transpose: 1
 */

int main (void)
{
    util::WallTimer timer;
    std::size_t timer_sfm_from_triplets1 = 0;
    std::size_t timer_sfm_from_triplets2 = 0;
    std::size_t timer_eigen_from_triplets1 = 0;
    std::size_t timer_eigen_from_triplets2 = 0;
    std::size_t timer_sfm_matrix_multiply_rm_cm = 0;
    std::size_t timer_sfm_matrix_multiply_cm_cm = 0;
    std::size_t timer_eigen_matrix_multiply = 0;
    std::size_t timer_sfm_matrix_transpose = 0;
    std::size_t timer_sfm_matrix_transpose2 = 0;
    std::size_t timer_eigen_matrix_transpose = 0;

    SparseMatrix mat0(MATRIX_ROWS, MATRIX_COLS, SparseMatrix::COLUMN_MAJOR);
    SparseMatrix mat1(MATRIX_ROWS, MATRIX_COLS, SparseMatrix::ROW_MAJOR);
    SparseMatrix mat2(MATRIX_ROWS, MATRIX_COLS, SparseMatrix::COLUMN_MAJOR);
    EigenMatrix mat3(MATRIX_ROWS, MATRIX_COLS);
    EigenMatrix mat4(MATRIX_ROWS, MATRIX_COLS);

    std::cout << "Generating triplets..." << std::endl;
    {
        SparseMatrix::Triplets sfm1_triplets, sfm2_triplets;
        EigenTriplets eigen_triplets;
        for (std::size_t i = 0; i < NON_ZERO_ENTRIES; ++i)
        {
            std::size_t row = std::rand() % MATRIX_ROWS;
            std::size_t col = std::rand() % MATRIX_COLS;
            double value = std::rand() / static_cast<double>(RAND_MAX);
            sfm1_triplets.emplace_back(row, col, value);
            sfm2_triplets.emplace_back(col, row, value);
            eigen_triplets.emplace_back(row, col, value);
        }

        mat0.set_from_triplets(&sfm1_triplets);

        timer.reset();
        mat1.set_from_triplets(&sfm1_triplets);
        timer_sfm_from_triplets1 = timer.get_elapsed();

        timer.reset();
        mat2.set_from_triplets(&sfm2_triplets);
        timer_sfm_from_triplets2 = timer.get_elapsed();

        timer.reset();
        mat3.setFromTriplets(eigen_triplets.begin(), eigen_triplets.end());
        timer_eigen_from_triplets1 = timer.get_elapsed();

        timer.reset();
        mat4.setFromTriplets(eigen_triplets.begin(), eigen_triplets.end());
        timer_eigen_from_triplets2 = timer.get_elapsed();
    }

    std::cout << "SfM Matrix Transpose..." << std::endl;
    timer.reset();
    SparseMatrix tmp3 = mat1.transpose();
    timer_sfm_matrix_transpose = timer.get_elapsed();

    std::cout << "SfM Matrix Transpose2..." << std::endl;
    timer.reset();
    SparseMatrix tmp6 = mat1.transpose2();
    timer_sfm_matrix_transpose2 = timer.get_elapsed();

    std::cout << "Eigen Matrix Transpose..." << std::endl;
    timer.reset();
    EigenMatrix tmp4 = mat3.transpose();
    timer_eigen_matrix_transpose = timer.get_elapsed();

    std::cout << "SfM Matrix RM-CM Multiply..." << std::endl;
    timer.reset();
    //SparseMatrix tmp1 = mat1.multiply(mat2);
    timer_sfm_matrix_multiply_rm_cm = timer.get_elapsed();

    std::cout << "SfM Matrix CM-CM Multiply..." << std::endl;
    timer.reset();
    SparseMatrix tmp5 = mat0.multiply(mat2);
    timer_sfm_matrix_multiply_cm_cm = timer.get_elapsed();

    std::cout << "Eigen Matrix Multiply..." << std::endl;
    timer.reset();
    EigenMatrix tmp2 = mat3 * mat4;
    timer_eigen_matrix_multiply = timer.get_elapsed();



    std::cout << "Timings" << std::endl
        << "  SfM Tiplets 1: " << timer_sfm_from_triplets1 << std::endl
        << "  SfM Tiplets 2: " << timer_sfm_from_triplets2 << std::endl
        << "  SfM RM CM Multiply: " << timer_sfm_matrix_multiply_rm_cm << std::endl
        << "  SfM CM CM Multiply: " << timer_sfm_matrix_multiply_cm_cm << std::endl
        << "  SfM Transpose 1: " << timer_sfm_matrix_transpose << std::endl
        << "  SfM Transpose 2: " << timer_sfm_matrix_transpose2 << std::endl
        << "  Eig Tiplets 1: " << timer_eigen_from_triplets1 << std::endl
        << "  Eig Tiplets 2: " << timer_eigen_from_triplets2 << std::endl
        << "  Eig Multiply: " << timer_eigen_matrix_multiply << std::endl
        << "  Eig Transpose: " << timer_eigen_matrix_transpose << std::endl;


    return 0;
}
