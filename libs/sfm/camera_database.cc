#include "camera_database.h"

SFM_NAMESPACE_BEGIN

CameraDatabase* CameraDatabase::instance = NULL;

namespace
{
    /**
     * Given a string, compute a simplified version that only contains upper
     * case alpha-numeric characters. All other characters are replaced with
     * spaces. Two or more subsequent spaces are replaced with a single space.
     */
    std::string
    simplify_string (std::string const& str)
    {
        std::string ret;

        bool was_alpha_numeric = true;
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            if (std::isalnum(str[i]))
            {
                if (!was_alpha_numeric)
                    ret.append(1, ' ');
                ret.append(1, std::toupper(str[i]));
                was_alpha_numeric = true;
            }
            else
                was_alpha_numeric = false;
        }

        return ret;
    }
}

void
CameraDatabase::add (std::string const& maker, std::string const& model,
    float sensor_width_mm, float sensor_height_mm,
    int sensor_width_px, int sensor_height_px)
{
    this->data.push_back(CameraModel());
    CameraModel& cam = this->data.back();
    cam.maker = simplify_string(maker);
    cam.model = simplify_string(model);
    cam.sensor_width_mm = sensor_width_mm;
    cam.sensor_height_mm = sensor_height_mm;
    cam.sensor_width_px = sensor_width_px;
    cam.sensor_height_px = sensor_height_px;
}

CameraModel const*
CameraDatabase::lookup (std::string const& maker,
    std::string const& model) const
{
    std::string s_maker = simplify_string(maker);
    std::string s_model = simplify_string(model);
    for (std::size_t i = 0; i < this->data.size(); ++i)
        if (this->data[i].maker == s_maker && this->data[i].model == s_model)
            return &this->data[i];
    return NULL;
}

CameraDatabase::CameraDatabase (void)
{
    // TODO: Add all cameras in the known universe.
    this->add("Canon", "Canon EOS 700D", 22.3f, 14.9f, 5184, 3456);
    this->add("LGE", "Nexus 5", 4.536f, 3.416f, 3264, 2448);
    this->add("LGE", "Nexus 4", 3.68f, 2.76f, 3264, 2448);
}

SFM_NAMESPACE_END
