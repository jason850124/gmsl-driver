//need to check

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/regmap.h>

#include "max_des.h"


//need to check
#define MAX96724_PHY1_ALT_CLOCK     5


struct max96724_chip_info
{
    unsigned int versions;
    bool supports_pipe_stream_autoseletct;
    unsigned int num_pipes;

    int (*set_pipe_tunnel_enable)(struct max_des *des, struct max_des_pipe *pipe, bool enable);

}

struct max96724_priv
{
    struct max_des des;
    const struct max96724_chip_info *inf;
    struct device *dev;
    struct i2c_client *clident;
    struct regmap *regmap;
    struct gpio_desc *gpio_enable;
    struct gpio_desc *gpiod_pwdn;
}


//need to check
static const struct max_phys_config max96724_phys_configs[] = 
{
    //D-PHY
    {.lanes = {2, 0, 2, 2}                   , .clock_lane = {MAX96724_PHY1_ALT_CLOCK, 0, 0, 0}},
    {.lanes = {2, 0, 4, 0}                   , .clock_lane = {MAX96724_PHY1_ALT_CLOCK, 0, 0, 0}},
    {.lanes = {2, 2, 2, 2}},
    {.lanes = {4, 0, 2, 2}},
    {.lanes = {2, 2, 4, 0}},
    {.lanes = {4, 0, 4, 0}},

    //C-PHY

};





static int max96724_init(struct max_des *des){
    struct max96724_priv *priv = des_to_priv(des);
    unsigned int i;
    int ret;

    if()
}

static const struct max_des_ops max96724 = 
{
    .num_phys = 4,
    .num_links = 4,
    .num_remaps_per_pipe = 16,   //need to check
    .phys_configs = 
    {
        .num_configs = ARRAY_SIZE(max96724_phys_configs),
        .configs = max96724_phys_configs,
    },
    .use_atr = true,
    .reg_read = max96724_reg_read,
    .reg_write = max96724_reg_write,
    .log_pipe_status = max96724_log_pipe_status,
    .log_phy_status = max96724_log_phy_status,
    //need to add whole functions
    .init = max96724_init,


};

static const struct max96724_chip_info max96724_info = 
{
    .versions = BIT(MAX_GMSL_2_3Gbps) | BIT(MAX_GMSL_2_6Gbps),
    .set_pipe_tunnel_enable = max96724_set_pipe_tunnel_enable,
    
}