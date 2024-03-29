#include "emitter.h"

spectral
Emitter::emission(const SampledLambdas &lambdas) const {
    return _emission.eval(lambdas);
}

// TODO: this will be more tricky for texture illuminants...
f32
Emitter::power() const {
    // Just use the rectangle rule...
    f32 sum = 0.f;
    constexpr u32 num_steps = 100;
    constexpr f32 lambda_min = static_cast<f32>(LAMBDA_MIN);
    constexpr f32 lambda_max = static_cast<f32>(LAMBDA_MAX);
    constexpr f32 h = (lambda_max - lambda_min) / static_cast<f32>(num_steps);
    for (u32 i = 0; i < num_steps; i++) {
        f32 lambda = lambda_min + (static_cast<f32>(i) * h) + h / 2.f;
        sum += _emission.eval_single(lambda);
    }

    f32 integral = sum * h;
    return integral / (lambda_max - lambda_min);
}
