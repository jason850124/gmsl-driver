#ifndef MAX_SERDES_H__
#define MAX_SERDES_H__

#define MAX_SERDES_PHYS_MAX    4

//lanes, clock lanes
struct max_phys_config {
    unsigned int lanes[MAX_SERDES_PHYS_MAX];
    unsigned int clock_lane[MAX_SERDES_PHYS_MAX];
};

//phys config, no.
struct max_phys_configs{
    const struct max_phys_config *configs;
    unsigned int num_configs;
};

#endif