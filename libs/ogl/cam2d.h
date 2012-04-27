#ifndef CAM2D_H
#define CAM2D_H

#include "math/vector.h"

#include "defines.h"
#include "events.h"
#include "camera.h"

OGL_NAMESPACE_BEGIN

class Cam2D
{
private:
    /* Camera information. */
    Camera* cam;

    float radius;
    math::Vec3f center;
    math::Vec2f mousePos;
    math::Vec3f tocam;
    math::Vec3f upvec;

public:
    Cam2D(void);

    void set_camera (Camera* camera);
    void consume_event (MouseEvent const& event);
    void consume_event(KeyboardEvent const& event);
    math::Vec3f get_campos(void);
    math::Vec3f get_viewdir(void);
    math::Vec3f get_upvec(void);
};

OGL_NAMESPACE_END

#endif // CAM2D_H
