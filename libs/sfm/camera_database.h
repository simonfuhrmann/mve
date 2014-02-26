#ifndef SFM_CAMERA_DATABASE_HEADER
#define SFM_CAMERA_DATABASE_HEADER

#include <vector>
#include <string>

#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Representation of a camera model.
 */
struct CameraModel
{
    std::string maker;
    std::string model;
    float sensor_width_mm;
    float sensor_height_mm;
    int sensor_width_px;
    int sensor_height_px;
};

/* ---------------------------------------------------------------- */

/**
 * Camera database which, given a maker and model string, will look for
 * a camera model in the database and return the model on successful lookup.
 * If the lookup fails, the database returns a NULL pointer.
 */
class CameraDatabase
{
public:
    static CameraDatabase* get (void);
    CameraModel const* lookup (std::string const& maker,
        std::string const& model) const;

private:
    CameraDatabase (void);
    void add (std::string const& maker, std::string const& model,
        float sensor_width_mm, float sensor_height_mm,
        int sensor_width_px, int sensor_height_px);

private:
    static CameraDatabase* instance;
    std::vector<CameraModel> data;
};

/* ------------------------ Implementation ------------------------ */

inline CameraDatabase*
CameraDatabase::get (void)
{
    if (CameraDatabase::instance == NULL)
        CameraDatabase::instance = new CameraDatabase();
    return CameraDatabase::instance;
}

SFM_NAMESPACE_END

#endif /* SFM_CAMERA_DATABASE_HEADER */
