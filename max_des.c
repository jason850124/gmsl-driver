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

    struct media_pad *pads;
    struct regulator **pocs;
	struct max_source *sources;
	u64 *streams_masks;

    struct notifier_block nb;
    struct v4l2_subdev sd;
    struct v4l2_async_notifier nf;

    struct max_des_phy *unused_phy;
};

int max_des_parse_dt(struct max_des_priv *priv){

    struct device *dev = priv->dev;
    

    return 0;

}

unsigned int max_des_num_pads(struct max_des *des){
    return des->ops->num_links+des->ops->num_phys;
}


int max_des_allocate(struct max_des_priv *priv){

    //we need to use priv and des, so first step we need to point to actual address;
    //and we also need to now actual pads' number of this device
    struct max_des *des = priv->des;
    unsigned int num_pads = max_des_num_pads(des);

    //distribute memories; and check if alloc is success or not.
    des->phys = devm_kcalloc(priv->dev, des->ops->num_phys, sizeof(*dev->phys), GFP_KERNEL);
    if(!des->phys)  
        return -ENOMEM;

    des->pipes = devm_kcalloc(priv->dev, des->ops->num_pipes, sizeof(*dev->pipes), GFP_KERNEL);
    if(!des->pipes)
        return -ENOMEM;

    des->links = devm_kcalloc(priv->dev, des->ops->num_links, sizeof(*dev->links), GFP_KERNEL);
    if(!des->links)
        return -ENOMEM;

    priv->pads = devm_kcalloc(priv->dev, num_pads, sizeof(*priv->pads), GFP_KERNEL);
    if(!priv->pads)
        return -ENOMEM;

    priv->sources = devm_kcalloc(priv->dev, des->ops->links, sizeof(*priv->sources), GFP_KERNEL);
    if(!priv->sources)
        return -ENOMEM;

    priv->pocs = devm_kcalloc(priv->dev, des->ops->links, sizeof(*priv->pocs), GFP_KERNEL);
    if(!priv->pocs)
        return -ENOMEM;

    priv->streams_masks = devm_kcalloc(priv->dev, num_pads, sizeof(*priv->streams_masks), GFP_KERNEL);
    if(!priv->streams_masks)
        return -ENOMEM;

    return 0;

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
    ret = max_des_parse_dt(priv);
    if(ret)
        return ret;

    //6. general deserializer init

    //7. manage power function

    //8. i2c init

    //9. register v4l2 subdev




    return 0;
}


int max_des_remove(){
    return 0;
}