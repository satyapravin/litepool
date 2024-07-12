#pragma once
#include <cmath>

#define NORMALIZE(struct_name, to_struct, mean_name, ssr_name, property_name, alpha) \
    do { \
        mean_name->property_name *= (1.0 - alpha); \
        mean_name->property_name += alpha * (struct_name.property_name - mean_name->property_name); \
        auto sqdelta = struct_name.property_name - mean_name->property_name; \
        ssr_name->property_name *= (1.0 - alpha); \
        ssr_name->property_name += alpha * sqdelta * sqdelta; \
        to_struct.property_name = sqdelta / std::pow(ssr_name->property_name + 1e-12, 0.5); \
    } while(0)