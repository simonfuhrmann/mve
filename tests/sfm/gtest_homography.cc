// Test cases for homography estimation.
// Written by Stepan Konrad.

#include <gtest/gtest.h>

#include "math/matrix_tools.h"
#include "sfm/homography.h"
#include "sfm/ransac_homography.h"
#include "sfm/correspondence.h"


namespace
{
    void
    fill_golden_correspondences(sfm::Correspondences2D2D& c,
        sfm::HomographyMatrix& H)
    {
        c[0].p1[0] =  0.214958928935434;  c[0].p1[1] = -0.906610909363408;
        c[1].p1[0] = -0.418432067487651;  c[1].p1[1] = -0.397661305085064;
        c[2].p1[0] =  0.479781912350596;  c[2].p1[1] = -0.991060231475749;
        c[3].p1[0] =  0.143409094445584;  c[3].p1[1] =  0.0274345038506874;
        c[4].p1[0] =  0.819362737875147;  c[4].p1[1] = -0.567577108916622;
        c[5].p1[0] =  0.414540477786197;  c[5].p1[1] =  0.214486790120107;
        c[6].p1[0] =  0.736728143446419;  c[6].p1[1] = -0.902682502921418;
        c[7].p1[0] = -0.679372773747302;  c[7].p1[1] = -0.304939170945405;
        c[0].p2[0] =  1.59306963823541;   c[0].p2[1] = -0.689071805581726;
        c[1].p2[0] =  0.785312853054716;  c[1].p2[1] = -0.777065157827566;
        c[2].p2[0] =  1.84004245395457;   c[2].p2[1] = -0.561528366527055;
        c[3].p2[0] =  0.882006419454985;  c[3].p2[1] = -0.0791953331223958;
        c[4].p2[0] =  1.78071457076429;   c[4].p2[1] = -0.0219606743581068;
        c[5].p2[0] =  0.941459319150067;  c[5].p2[1] =  0.244789446687795;
        c[6].p2[0] =  1.95923838519612;   c[6].p2[1] = -0.317347452952242;
        c[7].p2[0] =  0.535235460354669;  c[7].p2[1] = -0.896013650895132;

        double sin = std::sin(MATH_PI_4);
        double cos = std::cos(MATH_PI_4);
        H(0,0) = cos;   H(0,1) = -sin;  H(0,2) = 0.8;
        H(1,0) = sin;   H(1,1) = cos;   H(1,2) = -0.2;
        H(2,0) = 0.0;   H(2,1) = 0.0;   H(2,2) = 1.0;
    }
}  // namespace

TEST(HomographyTest, TestHomographyDLT)
{
    sfm::Correspondences2D2D c(8);
    sfm::HomographyMatrix H2;
    fill_golden_correspondences(c, H2);

    c.resize(4);

    sfm::HomographyMatrix H(0.0);
    sfm::homography_dlt(c, &H);

    H = H / H[8];

    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((H[i] - H2[i]), 0.0, 1e-6);
}

TEST(HomographyTest, TestHomographyDLTLeastSquares)
{
    sfm::Correspondences2D2D c(8);
    sfm::HomographyMatrix H2;
    fill_golden_correspondences(c, H2);

    sfm::HomographyMatrix H(0.0);
    sfm::homography_dlt(c, &H);
    H = H / H[8];

    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((H[i] - H2[i]), 0.0, 1e-6);
}

TEST(HomographyTest, TestSymmetricTransferError)
{
    sfm::Correspondences2D2D c(8);
    sfm::HomographyMatrix H2;
    fill_golden_correspondences(c, H2);

    for (std::size_t i = 0; i < c.size(); ++i)
    {
        sfm::Correspondence2D2D match = c[i];
        double error = sfm::symmetric_transfer_error(H2, match);
        EXPECT_NEAR(error, 0.0, 1e-16);
    }
}

TEST(RansacHomographyTest, TestEstimate)
{
    sfm::Correspondences2D2D c(8);
    sfm::HomographyMatrix H2;
    fill_golden_correspondences(c, H2);

    sfm::RansacHomography::Options options;
    sfm::RansacHomography ransac(options);
    sfm::RansacHomography::Result result;
    ransac.estimate(c, &result);

    sfm::HomographyMatrix H = result.homography;
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((H[i] - H2[i]), 0.0, 1e-6);
}
