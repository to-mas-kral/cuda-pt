#include "rough_plastic.h"

#include "../math/sampling.h"
#include "common.h"
#include "trowbridge_reitz_ggx.h"

f32
RoughPlasticMaterial::pdf(const ShadingGeometry &sgeom, const SampledLambdas &λ) const {
    f32 int_η = int_ior.eval_single(λ[0]);
    f32 ext_η = ext_ior.eval_single(λ[0]);
    f32 rel_η = int_η / ext_η;

    f32 fresnel_i = fresnel_dielectric(rel_η, sgeom.nowo);

    f32 diffuse_pdf = (1.f - fresnel_i) * sgeom.cos_theta / M_PIf;
    f32 microfacet_pdf = fresnel_i * TrowbridgeReitzGGX::pdf(sgeom, alpha);

    return diffuse_pdf + microfacet_pdf;
}

spectral
RoughPlasticMaterial::eval(const ShadingGeometry &sgeom, const SampledLambdas &lambdas,
                           const Texture *textures, const vec2 &uv) const {
    f32 int_ior_s = int_ior.eval_single(lambdas[0]);
    f32 ext_ior_s = ext_ior.eval_single(lambdas[0]);
    /// This is external / internal !
    f32 rel_ior = ext_ior_s / int_ior_s;

    f32 fresnel_i = fresnel_dielectric(1.f / rel_ior, sgeom.nowi);

    // Specular case
    f32 D = TrowbridgeReitzGGX::D(sgeom.noh, alpha);

    float G = TrowbridgeReitzGGX::G1(sgeom.nowi, sgeom.howo, alpha) *
              TrowbridgeReitzGGX::G1(sgeom.nowo, sgeom.howo, alpha);
    spectral microfacet_brdf =
        spectral::make_constant(fresnel_i * G * D) / (4.f * sgeom.nowo * sgeom.nowi);

    // TODO: try the height-correlated smith
    /*f32 V = visibility_smith_height_correlated_ggx(sgeom.nowo, sgeom.nowi,
    alpha); return fresnel * V * D;*/

    // Figure out the angle of the ray that refracts from inside to outisde ωo
    f32 cos_theta_out = sgeom.nowo;
    f32 sin_theta_out = std::sqrt(1.f - sqr(cos_theta_out));
    f32 sin_theta_in = sin_theta_out * rel_ior;
    f32 cos_theta_in = std::sqrt(1.f - sqr(sin_theta_in));
    f32 fresnel_o = fresnel_dielectric(rel_ior, cos_theta_in);

    const Texture *texture = &textures[diffuse_reflectance_id];
    tuple3 refl_sigmoid_coeff = texture->fetch(uv);
    spectral α = RgbSpectrum::from_coeff(refl_sigmoid_coeff).eval(lambdas);

    f32 re = 0.919317f;
    f32 ior_pow = int_ior_s;
    re -= (3.4793f / ior_pow);
    ior_pow *= ior_pow;
    re += (6.75335f / ior_pow);
    ior_pow *= ior_pow;
    re -= (7.80989f / ior_pow);
    ior_pow *= ior_pow;
    re += (4.98554f / ior_pow);
    ior_pow *= ior_pow;
    re -= (1.36881f / ior_pow);

    f32 ri = 1.f - sqr(rel_ior) * (1.f - re);

    spectral scattering_brdf = α * (1.f - fresnel_i) * (1.f - fresnel_o) /
                               ((spectral::make_constant(1.f) - α * ri) * M_PIf);

    spectral brdf = microfacet_brdf + scattering_brdf * sqr(rel_ior);

    return brdf;
}

Option<BSDFSample>
RoughPlasticMaterial::sample(const norm_vec3 &normal, const norm_vec3 &ωo, const vec3 &ξ,
                             const SampledLambdas &λ, const Texture *textures,
                             const vec2 &uv) const {
    f32 int_η = int_ior.eval_single(λ[0]);
    f32 ext_η = ext_ior.eval_single(λ[0]);
    f32 rel_η = int_η / ext_η;

    f32 noωo = vec3::dot(normal, ωo);
    f32 potential_fresnel = fresnel_dielectric(rel_η, noωo);

    if (ξ.z < potential_fresnel) {
        // sample specular
        norm_vec3 wi = TrowbridgeReitzGGX::sample(normal, ωo, vec2(ξ.x, ξ.y), alpha);
        auto sgeom = ShadingGeometry::make(normal, wi, ωo);

        if (sgeom.nowi * sgeom.nowo <= 0.f) {
            return {};
        }

        return BSDFSample{
            .bsdf = eval(sgeom, λ, textures, uv),
            .wi = wi,
            .pdf = pdf(sgeom, λ),
        };
    } else {
        // sample diffuse
        norm_vec3 sample_dir = sample_cosine_hemisphere(vec2(ξ.x, ξ.y));
        norm_vec3 ωi = transform_frame(sample_dir, normal);
        auto sgeom = ShadingGeometry::make(normal, ωi, ωo);

        return BSDFSample{
            .bsdf = eval(sgeom, λ, textures, uv),
            .wi = ωi,
            .pdf = pdf(sgeom, λ),
        };
    }
}
