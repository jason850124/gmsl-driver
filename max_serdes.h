#ifndef MAX_SERDES_H__
#define MAX_SERDES_H__

#include <linux/types.h>
#include <media/v4l2-subdev.h>


#define MAX_SERDES_PHYS_MAX    4

//need check

//v4l2_subdev, pad no., endpoint, index(remote device, pad number in this device, endpoint for remote, index in this device?) need check
struct max_source
{
    struct v4l2_subdev *sd;
    u64 pad;
    struct fwnode_handle *ep_fwnode;
    unsigned int index;
}

//lanes, clock lanes
struct max_phys_config
{
    unsigned int lanes[MAX_SERDES_PHYS_MAX];
    unsigned int clock_lane[MAX_SERDES_PHYS_MAX];
};

//phys config, no.
struct max_phys_configs
{
    const struct max_phys_config *configs;
    unsigned int num_configs;
};

#endif