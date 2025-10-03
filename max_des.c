#include <linux/delay.h>
#include <linux/i2c-atr.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <media/mipi-csi2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>


#include "max_des.h"


#define MAX_DES_PHYS_NUM       4
#define MAX_DES_PIPES_NUM      8


struct max_des_priv{
    struct max_des *des;

    struct device *dev;
    struct i2c_client *client;
    struct i2c_atr *atr;
    struct i2c_mux_core *mux;

    //struct media_pad *pads;
    //struct regulator **pocs;
	struct max_source *sources;
	u64 *streams_masks;

    struct notifier_block nb;
    struct v4l2_subdev sd;
    struct v4l2_async_notifier nf;

    struct max_des_phy *unused_phy;
};


int max_des_alloc(struct max_des_priv *priv){

    #

}


// This function is used to probe device into Linux kernel.
int max_des_probe(struct i2c_client *client, struct max_des *des){

    //0. get needed pointers
    struct device *dev = &client->dev;
    struct max_des_priv *priv = des->priv;
    int ret;

    //1. check numbers of device pads
    if (des->ops->num_phys > MAX_DES_PHYS_NUM)
        return -E2BIG;

    if(des->ops->num_pipes > MAX_DES_PIPES_NUM)
        return -E2BIG;

    if(des->ops->num_links > des->ops->num_pipes)
        return -E2BIG;

    //2. distribute memory resource for priv
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);

    //3. connect each structures (make each structures work together)
    priv->client = client;
    priv->dev = dev;
    priv->des = des;
    des->priv = priv;
    
    //4. distribute memory resources to entities of the device
    ret = max_des_allocate(priv);
    if(ret)
        return ret;
    //5. parse device tree information

    //6. general deserializer init

    //7. manage power function

    //8. i2c init

    //9. register v4l2 subdev




    return 0;
}


int max_des_remove(){
    return 0;
}