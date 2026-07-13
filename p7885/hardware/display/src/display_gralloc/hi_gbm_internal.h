#ifndef P7885_HI_GBM_INTERNAL_H
#define P7885_HI_GBM_INTERNAL_H

#include <cstdint>

#define DIV_ROUND_UP_GBM(n, d) (((n) + (d)-1) / (d))
#define ALIGN_UP_GBM(x, a) ((((x) + ((a)-1)) / (a)) * (a))
#define HEIGHT_ALIGN_GBM 2U
#define WIDTH_ALIGN_GBM 8U
#define MAX_PLANES_GBM 3

struct gbm_device {
    int fd;
};

struct gbm_bo {
    struct gbm_device *gbm;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t handle;
    uint32_t stride;
    uint32_t size;
};

#endif // P7885_HI_GBM_INTERNAL_H
