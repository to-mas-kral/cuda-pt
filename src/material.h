#ifndef PT_MATERIAL_H
#define PT_MATERIAL_H

#include "math/sampling.h"
#include "math/vecmath.h"
#include "utils/numtypes.h"
#include "utils/rng.h"

class Material {
public:
    explicit Material(const vec3 &reflectance) : reflectance(reflectance) {}
    explicit Material(u32 reflectance_tex_id) : reflectance_tex_id(reflectance_tex_id) {}

    __device__ vec3 sample(vec3 normal, vec3 view_dir, vec2 sample) const {
        vec3 sample_dir = sample_cosine_hemisphere(sample);
        return orient_dir(sample_dir, normal);
    }

    // Probability density function of sampling the BRDF
    __device__ f32 pdf(f32 cos_theta) const { return cos_theta / M_PIf; }

    __device__ vec3 eval(const Material *mat, Texture *textures, vec2 uv) const {
        vec3 refl = reflectance;
        if (mat->reflectance_tex_id.has_value()) {
            auto tex_id = mat->reflectance_tex_id.value();
            auto texture = &textures[tex_id];
            refl = vec3(texture->sample(uv));
        }

        return refl / M_PIf;
    }
    vec3 reflectance = vec3(0.);
    cuda::std::optional<u32> reflectance_tex_id = cuda::std::nullopt;
};

#endif // PT_MATERIAL_H
