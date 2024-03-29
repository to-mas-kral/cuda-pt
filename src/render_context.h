#ifndef PT_RENDER_CONTEXT_H
#define PT_RENDER_CONTEXT_H

#include "camera.h"
#include "framebuffer.h"
#include "io/scene_loader.h"
#include "scene/scene.h"
#include "utils/basic_types.h"

/// Render Context is a collection of data needed for the integrators to do their job.
class RenderContext {
public:
    explicit RenderContext(SceneAttribs &attribs) : attribs(attribs) {
        f32 aspect = static_cast<f32>(attribs.resx) / static_cast<f32>(attribs.resy);
        cam = Camera(attribs.fov, aspect);
        fb = Framebuffer(attribs.resx, attribs.resy);
    }

    Scene scene{};
    Camera cam;
    Framebuffer fb;
    SceneAttribs attribs;
};

#endif // PT_RENDER_CONTEXT_H
