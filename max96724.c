//need to check
//need to note


#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/regmap.h>

#include "max_des.h"


//need to check
#define MAX96724_REG0				0x0

#define MAX96724_REG6				0x6
#define MAX96724_REG6_LINK_EN			GENMASK(3, 0)

#define MAX96724_REG26(x)			(0x10 + (x) / 2)
#define MAX96724_REG26_RX_RATE_PHY(x)		(GENMASK(1, 0) << (4 * ((x) % 2)))
#define MAX96724_REG26_RX_RATE_3Gbps		0b01
#define MAX96724_REG26_RX_RATE_6Gbps		0b10

#define MAX96724_PWR1				0x13
#define MAX96724_PWR1_RESET_ALL			BIT(6)

#define MAX96724_CTRL1				0x18
#define MAX96724_CTRL1_RESET_ONESHOT		GENMASK(3, 0)

#define MAX96724_VIDEO_PIPE_SEL(p)		(0xf0 + (p) / 2)
#define MAX96724_VIDEO_PIPE_SEL_STREAM(p)	(GENMASK(1, 0) << (4 * ((p) % 2)))

#define MAX96724_VIDEO_PIPE_EN			0xf4
#define MAX96724_VIDEO_PIPE_EN_MASK(p)		BIT(p)
#define MAX96724_VIDEO_PIPE_EN_STREAM_SEL_ALL	BIT(4)

#define MAX96724_VPRBS(p)			(0x1dc + (p) * 0x20)
#define MAX96724_VPRBS_VIDEO_LOCK		BIT(0)

#define MAX96724_BACKTOP12			0x40b
#define MAX96724_BACKTOP12_CSI_OUT_EN		BIT(1)

#define MAX96724_BACKTOP21			0x414
#define MAX96724_BACKTOP21_BPP8DBL(p)		BIT(4 + (p))

#define MAX96724_BACKTOP22(x)			(0x415 + (x) * 0x3)
#define MAX96724_BACKTOP22_PHY_CSI_TX_DPLL	GENMASK(4, 0)
#define MAX96724_BACKTOP22_PHY_CSI_TX_DPLL_EN	BIT(5)

#define MAX96724_BACKTOP24			0x417
#define MAX96724_BACKTOP24_BPP8DBL_MODE(p)	BIT(4 + (p))

#define MAX96724_BACKTOP30			0x41d
#define MAX96724_BACKTOP30_BPP10DBL3		BIT(4)
#define MAX96724_BACKTOP30_BPP10DBL3_MODE	BIT(5)

#define MAX96724_BACKTOP31			0x41e
#define MAX96724_BACKTOP31_BPP10DBL2		BIT(6)
#define MAX96724_BACKTOP31_BPP10DBL2_MODE	BIT(7)

#define MAX96724_BACKTOP32			0x41f
#define MAX96724_BACKTOP32_BPP12(p)		BIT(p)
#define MAX96724_BACKTOP32_BPP10DBL0		BIT(4)
#define MAX96724_BACKTOP32_BPP10DBL0_MODE	BIT(5)
#define MAX96724_BACKTOP32_BPP10DBL1		BIT(6)
#define MAX96724_BACKTOP32_BPP10DBL1_MODE	BIT(7)

#define MAX96724_MIPI_PHY0			0x8a0
#define MAX96724_MIPI_PHY0_PHY_CONFIG		GENMASK(4, 0)
#define MAX96724_MIPI_PHY0_PHY_4X2		BIT(0)
#define MAX96724_MIPI_PHY0_PHY_2X4		BIT(2)
#define MAX96724_MIPI_PHY0_PHY_1X4A_2X2		BIT(3)
#define MAX96724_MIPI_PHY0_PHY_1X4B_2X2		BIT(4)

#define MAX96724_MIPI_PHY2			0x8a2
#define MAX96724_MIPI_PHY2_PHY_STDB_N_4(x)	(GENMASK(5, 4) << ((x) / 2 * 2))
#define MAX96724_MIPI_PHY2_PHY_STDB_N_2(x)	(BIT(4 + (x)))

#define MAX96724_MIPI_PHY3(x)			(0x8a3 + (x) / 2)
#define MAX96724_MIPI_PHY3_PHY_LANE_MAP_4	GENMASK(7, 0)
#define MAX96724_MIPI_PHY3_PHY_LANE_MAP_2(x)	(GENMASK(3, 0) << (4 * ((x) % 2)))

#define MAX96724_MIPI_PHY5(x)			(0x8a5 + (x) / 2)
#define MAX96724_MIPI_PHY5_PHY_POL_MAP_4_0_1	GENMASK(1, 0)
#define MAX96724_MIPI_PHY5_PHY_POL_MAP_4_2_3	GENMASK(4, 3)
#define MAX96724_MIPI_PHY5_PHY_POL_MAP_4_CLK	BIT(5)
#define MAX96724_MIPI_PHY5_PHY_POL_MAP_2(x)	(GENMASK(1, 0) << (3 * ((x) % 2)))
#define MAX96724_MIPI_PHY5_PHY_POL_MAP_2_CLK(x)	BIT(2 + 3 * ((x) % 2))

#define MAX96724_MIPI_PHY13			0x8ad
#define MAX96724_MIPI_PHY13_T_T3_PREBEGIN	GENMASK(5, 0)
#define MAX96724_MIPI_PHY13_T_T3_PREBEGIN_64X7	FIELD_PREP(MAX96724_MIPI_PHY13_T_T3_PREBEGIN, 63)

#define MAX96724_MIPI_PHY14			0x8ae
#define MAX96724_MIPI_PHY14_T_T3_PREP		GENMASK(1, 0)
#define MAX96724_MIPI_PHY14_T_T3_PREP_55NS	FIELD_PREP(MAX96724_MIPI_PHY14_T_T3_PREP, 0b01)
#define MAX96724_MIPI_PHY14_T_T3_POST		GENMASK(6, 2)
#define MAX96724_MIPI_PHY14_T_T3_POST_32X7	FIELD_PREP(MAX96724_MIPI_PHY14_T_T3_POST, 31)

#define MAX96724_MIPI_CTRL_SEL			0x8ca
#define MAX96724_MIPI_CTRL_SEL_MASK(p)		(GENMASK(1, 0) << ((p) * 2))

#define MAX96724_MIPI_PHY25(x)			(0x8d0 + (x) / 2)
#define MAX96724_MIPI_PHY25_CSI2_TX_PKT_CNT(x)	(GENMASK(3, 0) << (4 * ((x) % 2)))

#define MAX96724_MIPI_PHY27(x)			(0x8d2 + (x) / 2)
#define MAX96724_MIPI_PHY27_PHY_PKT_CNT(x)	(GENMASK(3, 0) << (4 * ((x) % 2)))

#define MAX96724_MIPI_TX3(x)			(0x903 + (x) * 0x40)
#define MAX96724_MIPI_TX3_DESKEW_INIT_8X32K	FIELD_PREP(GENMASK(2, 0), 0b001)
#define MAX96724_MIPI_TX3_DESKEW_INIT_AUTO	BIT(7)

#define MAX96724_MIPI_TX4(x)			(0x904 + (x) * 0x40)
#define MAX96724_MIPI_TX4_DESKEW_PER_2K		FIELD_PREP(GENMASK(2, 0), 0b001)
#define MAX96724_MIPI_TX4_DESKEW_PER_AUTO	BIT(7)

#define MAX96724_MIPI_TX10(x)			(0x90a + (x) * 0x40)
#define MAX96724_MIPI_TX10_CSI2_CPHY_EN		BIT(5)
#define MAX96724_MIPI_TX10_CSI2_LANE_CNT	GENMASK(7, 6)

#define MAX96724_MIPI_TX11(p)			(0x90b + (p) * 0x40)
#define MAX96724_MIPI_TX12(p)			(0x90c + (p) * 0x40)

#define MAX96724_MIPI_TX13(p, x)		(0x90d + (p) * 0x40 + (x) * 0x2)
#define MAX96724_MIPI_TX13_MAP_SRC_DT		GENMASK(5, 0)
#define MAX96724_MIPI_TX13_MAP_SRC_VC		GENMASK(7, 6)

#define MAX96724_MIPI_TX14(p, x)		(0x90e + (p) * 0x40 + (x) * 0x2)
#define MAX96724_MIPI_TX14_MAP_DST_DT		GENMASK(5, 0)
#define MAX96724_MIPI_TX14_MAP_DST_VC		GENMASK(7, 6)

#define MAX96724_MIPI_TX45(p, x)		(0x92d + (p) * 0x40 + (x) / 4)
#define MAX96724_MIPI_TX45_MAP_DPHY_DEST(x)	(GENMASK(1, 0) << (2 * ((x) % 4)))

#define MAX96724_MIPI_TX51(x)			(0x933 + (x) * 0x40)
#define MAX96724_MIPI_TX51_ALT_MEM_MAP_12	BIT(0)
#define MAX96724_MIPI_TX51_ALT_MEM_MAP_8	BIT(1)
#define MAX96724_MIPI_TX51_ALT_MEM_MAP_10	BIT(2)
#define MAX96724_MIPI_TX51_ALT2_MEM_MAP_8	BIT(4)

#define MAX96724_MIPI_TX54(x)			(0x936 + (x) * 0x40)
#define MAX96724_MIPI_TX54_TUN_EN		BIT(0)

#define MAX96724_MIPI_TX57(x)			(0x939 + (x) * 0x40)
#define MAX96724_MIPI_TX57_TUN_DEST		GENMASK(5, 4)
#define MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET	BIT(6)

#define MAX96724_DE_DET				0x11f0
#define MAX96724_HS_DET				0x11f1
#define MAX96724_VS_DET				0x11f2
#define MAX96724_HS_POL				0x11f3
#define MAX96724_VS_POL				0x11f4
#define MAX96724_DET(p)				BIT(p)

#define MAX96724_DPLL_0(x)			(0x1c00 + (x) * 0x100)
#define MAX96724_DPLL_0_CONFIG_SOFT_RST_N	BIT(0)

#define MAX96724_PHY1_ALT_CLOCK		5

#define field_get(mask, val) (((val) & (mask)) >> __ffs(mask))
#define field_prep(mask, val) (((val) << __ffs(mask)) & (mask))


#define des_to_priv(_des)  container_of(_des, max96724_priv, des)

static const struct regmap_config max96724_i2c_regmap = 
{
    .reg_bits = 16,
    .val_bits = 8,
    .max_register = 0x1271, //default: 0x1f00, refer to datasheet 0x1271 is the latest register
};


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
    const struct max96724_chip_info *info;
    struct device *dev;
    struct i2c_client *client;
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

//need to check max_des_ops functions, we used des as an input parameter that is because functions will be called in max_des.c
static int max96724_reg_read(struct max_des *des, unsigned int reg, unsigned int *val)
{   
    struct max96724_priv *priv = des_to_priv(des);
    return regmap_read(priv->regmap, reg, val);
}

static int max96724_reg_write(struct max_des *des, unsigned int reg, unsigned int val)
{
    struct max96724_priv *priv = des_to_priv(des);
    return regmap_write(priv->regmap, reg, val);
}

static int max96724_log_pipe_status(struct max_des* des, struct max_des_pipe *pipe, const char *name)
{
    struct max96724_priv *priv = des_to_priv(des);
    unsigned int index = pipe->index;
    unsigned int val, mask;
    int ret;

	ret = regmap_read(priv->regmap, MAX96724_VPRBS(index), &val);
	if (ret)
		return ret;

	pr_info("%s: \tvideo_lock: %u\n", name,
		!!(val & MAX96724_VPRBS_VIDEO_LOCK));

	mask = MAX96724_DET(index);

	ret = regmap_read(priv->regmap, MAX96724_DE_DET, &val);
	if (ret)
		return ret;

	pr_info("%s: \tde_det: %u\n", name, !!(val & mask));

	ret = regmap_read(priv->regmap, MAX96724_HS_DET, &val);
	if (ret)
		return ret;

	pr_info("%s: \ths_det: %u\n", name, !!(val & mask));

	ret = regmap_read(priv->regmap, MAX96724_VS_DET, &val);
	if (ret)
		return ret;

	pr_info("%s: \tvs_det: %u\n", name, !!(val & mask));

	ret = regmap_read(priv->regmap, MAX96724_HS_POL, &val);
	if (ret)
		return ret;

	pr_info("%s: \ths_pol: %u\n", name, !!(val & mask));

	ret = regmap_read(priv->regmap, MAX96724_VS_POL, &val);
	if (ret)
		return ret;

	pr_info("%s: \tvs_pol: %u\n", name, !!(val & mask));

	return 0;
}


int max96724_set_pipe_tunnel_enable(struct max_des *des, struct max_des_pipe *pipe, bool enable)
{
    struct max96724_priv* priv = des_to_priv(des);
    return regmap_assign_bits(priv->regmap, MAX96724_MIPI_TX54(pipe->index), MAX96724_MIPI_TX54_TUN_EN,enable);
}



static int max96724_init(struct max_des *des){
    struct max96724_priv *priv = des_to_priv(des);
    unsigned int i;
    int ret;

}

static const struct max_des_ops max96724 = 
{
    .num_phys = 4,
    .num_links = 4,
    .num_remaps_per_pipe = 16, 
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
	.set_enable = max96724_set_enable,
	.init = max96724_init,
	.init_phy = max96724_init_phy,
	.set_phy_mode = max96724_set_phy_mode,
	.set_phy_active = max96724_set_phy_active,
	.set_pipe_phy = max96724_set_pipe_phy,
	.set_pipe_stream_id = max96724_set_pipe_stream_id,
	.set_pipe_enable = max96724_set_pipe_enable,
	.set_pipe_remap = max96724_set_pipe_remap,
	.set_pipe_remaps_enable = max96724_set_pipe_remaps_enable,
	.set_pipe_mode = max96724_set_pipe_mode,
	.select_links = max96724_select_links,
	.set_link_version = max96724_set_link_version,

};

static const struct max96724_chip_info max96724_info = 
{
    .versions = BIT(MAX_GMSL_2_3Gbps) | BIT(MAX_GMSL_2_6Gbps),
    .set_pipe_tunnel_enable = max96724_set_pipe_tunnel_enable,
    .supports_pipe_stream_autoseletct = true,
    .num_pipes = 4,
}


/* here define for max96724f
static const struct max96724_chip_info max96724f_info = 
{
    .versions = BIT(MAX_GMSL_2_3Gbps),
    .set_pipe_tunnel_enable = max96724_set_pipe_tunnel_enable,
    .supports_pipe_stream_autoseletct = true,
    .num_pipes = 4,
}

*/






//wait for SoC that can read right device address.
static int max96724_wait_for_device(struct max96724_priv *priv)
{   
    int ret;

    for(i = 0; i < 10; i++)
    {
        unsigned int val = 0;
        /*MAX96724_REG0 : 7-1: device address 0: block*/
        ret = regmap_read(priv->regmap, MAX96724_REG0, &val);
        if(!ret && (val >> 1) == priv->client->addr)
            return 0;

        msleep(100);

        dev_err(priv->dev, "Retry %u waiting for deserializer: ret->%d, val->%d\n",i , ret, val);
    }

    return ret;
}

//reset chip operation
static int max96724_reset(struct max96724_priv *priv)
{

    int ret;

    ret = max96724_wait_for_device(priv);
    if(ret)
        return ret;

    ret = regmap_update_bits(priv->regmap, MAX96724_PWR1, MAX96724_PWR1_RESET_ALL,
                             FIELD_PREP(MAX96724_PWR1_RESET_ALL, 1));
    if(ret)
        return ret;

    fsleep(10000);

    ret = max96724_wait_for_device(priv);
    if(ret)
        return ret;

    return 0;
}


/*probe function to start everything*/
static int max96724_probe(struct i2c_client *client)
{   
    /*declare necessary structures*/
    struct device *dev = &client->dev;
    struct max96724_priv *priv;
    struct max_des_ops *ops;
    
    /*1. distribute memories*/
    priv = devm_kzalloc(priv->dev, sizeof(*priv), GFP_KERNEL);
    if(!priv)
        return -ENOMEM;

    ops = devm_kzalloc(priv->dev, sizeof(*ops), GFP_KERNEL);
    if(!ops)
        return -ENOMEM;
    /*get device's info from corresponding "of match table"*/
    priv->info = device_get_match_data(dev);
    if(!priv->info)
    {
        dev_err(dev, "Failed to get match data\n");
        return -ENODEV;
    }

    /*connect each other*/
    priv->dev = dev;
    priv->client = client;
    i2c_set_clientdata(client, priv);

    //connect i2c to regmap with max96724 format and send it to priv structure
    priv->regmap = devm_regmap_init_i2c(client, &max96724_i2c_regmap);
    if(IS_ERR(priv->regmap))
        return PTR_ERR(priv->regmap);

    //need to note this pin
    priv->gpiod_enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_HIGH);
    if(IS_ERR(priv->gpiod_enable))
        return PTR_ERR(priv->gpiod_enable);

    if(priv->gpio_enable)
        usleep_range(4000, 5000);


    priv->gpiod_pwdn = devm_gpiod_get_optional(dev, "powerdown", GPIOD_OUT_HIGH);
    if(IS_ERR(priv->gpiod_pwdn))
        return PTR_ERR(priv->gpiod_pwdn);

    if(priv->gpiod_pwdn)
    {
        udelay(1);
        
        gpiod_set_value_cansleep(priv->gpiod_pwdn, 0);
        usleep_range(4000, 5000);
    }


    *ops = max96724_ops;
    ops->version = priv->info->versions;
    ops->num_pipes = priv->info->num_pipes;
    ops->set_pipe_tunnel_enable = priv->info->set_pipe_tunnel_enable;
    priv->des.ops = ops;

    ret = max96724_reset(priv);
    if(ret)
        return ret;

    return max_des_probe(client, &priv->des);
    
}

/*oppsite of probe is remove*/
static void max96724_remove(struct i2c_client *client)
{
    struct max96724_priv *priv = i2c_get_clientdata(client);

    /*need to note, we should create this function*/
    max_des_remove(&priv->des);

    gpiod_set_value_cansleep(priv->gpiod_pwdn, 1);
    gpiod_set_value_cansleep(priv->gpiod_enable, 0);
}

//just test 96724 first
static const struct of_device_id max96724_of_table[] = 
{
    //{ .compatible = "maxim,max96712",  .data=&max96712_info},
    { .compatible = "maxim,max96724",  .data=&max96724_info},
    //{ .compatible = "maxim,max96724f", .data=&max96724f_info},
    //{ .compatible = "maxim,max96724r", .data=&max96724f_info}
};

//this is use for denoting ID table to kernel module
MODULE_DEVICE_TABLE(of, max96724_of_table);

static struct i2c_driver max96724_i2c_driver = 
{
    .driver = 
    {
        .name = "max96724",
        .of_match_table = of_match_ptr(max96724_of_table),
    },
    .probe = max96724_probe,
    .remove = max96724_remove,
};

module_i2c_driver(max96724_i2c_driver);

MODULE_DESCRIPTION("Analog devices MAX96724 Quad GMSL2 Deserializer Driver");
MODULE_AUTHOR("Cosmin Tanislav <cosmin.tanislav@analog.com>");
MODULE_AUTHOR("Jason Lai <jason.lai@analog.com>");
MODULE_LICENSE("GPL");