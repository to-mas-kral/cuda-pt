
#include "framebuffer.h"

// TODO: might be better to just copy some PCG implementation, this uses a lot of space...
__global__ void init_rand_state(u32 pixels, u32 image_x, curandState *curand_state) {
    // This is the same as pixel_index() which can't be used because Framebuffer object
    // is still being created.
    auto x = (blockIdx.x * blockDim.x) + threadIdx.x;
    auto y = (blockIdx.y * blockDim.y) + threadIdx.y;
    auto pixel_index = (y * image_x) + x;

    if (pixel_index < pixels) {
        curand_init(1984 + pixel_index, 0, 0, &curand_state[pixel_index]);
    }
}

Framebuffer::Framebuffer(u32 image_x, u32 image_y, dim3 blocks_dim, dim3 threads_dim)
    : image_x(image_x), image_y(image_y) {
    pixels = SharedVector<vec3>(vec3(0.f, 0.f, 0.f), num_pixels());
    rand_state = SharedVector<curandState>(num_pixels());

    // RenderContext isn't initialized yet. Have to pass rand_state raw pointer,
    // because rand_state itself could still be placed on the stack at this point.
    init_rand_state<<<blocks_dim, threads_dim>>>(num_pixels(), image_x,
                                                 rand_state.get_ptr());
    cudaDeviceSynchronize();
    CUDA_CHECK_LAST_ERROR();

    rand_state.assume_all_init();
}

__device__ cuda::std::tuple<u64, u64>
Framebuffer::pixel_coords(dim3 block_dim, dim3 block_idx, dim3 thread_idx) const {
    auto x = (block_idx.x * block_dim.x) + thread_idx.x;
    auto y = (block_idx.y * block_dim.y) + thread_idx.y;

    return {x, y};
}

__device__ u64 Framebuffer::pixel_index(u64 x, u64 y) const {
    return ((image_y - 1U - y) * image_x) + x;
}

__device__ u64 Framebuffer::pixel_index(dim3 block_dim, dim3 block_idx,
                                        dim3 thread_idx) const {
    auto [x, y] = pixel_coords(block_dim, block_idx, thread_idx);
    return pixel_index(x, y);
}