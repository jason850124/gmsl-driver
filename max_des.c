#include <linux/delay.h>
#include <linux/i2c-atr.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <media/mipi-csi2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>


#include "max_des.h"

#define MAX_DES_LINK_FREQUENCY_MIN 100000000ull
#define MAX_DES_LINK_FREQUENCY_DEFAULT 750000000ull
#define MAX_DES_LINK_FREQUENCY_MAX 1250000000ull


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

static int max_des_phy_to_pad(struct max_des *des, struct max_des_phy *phy)
{
    return des->ops->num_links + phy->index;
}

static int max_des_parse_src_dt(struct max_des_priv *priv, 
                                struct max_des_phy *phy,
                                struct fwnode_handle *fwnode)
{
    struct max_des *des = priv->des;
    struct v4l2_fwnode_endpoint v4l2_ep = {.bus = V4L2_MBUS_UNKNOWN};
    struct v4l2_mbus_config_mipi_csi2 *mipi = &v4l2_ep.bus.mipi_csi2;
    enum v4l2_mbus_type bus_type;
    struct fwnode_handle *ep;
    u64 link_frequency;
    unsigned int i;
    int ret;

    u32 pad = max_des_phy_to_pad(des,phy);

    ep = fwnode_graph_get_endpoint_by_id(fwnode, pad, 0,0);
    if(!ep)
        return 0;

    ret = v4l2_fwnode_endpoint_alloc_parse(ep, &v4l2_ep);
    fwnode_handle_put(ep);
    if(ret)
    {
        dev_err(priv->dev, "Could not parse endpoint on port %u\n", pad);
        return ret;
    }

    bus_type = v4l2_ep.bus_type;
    if(bus_type != V4L2_MBUS_CSI2_DPHY &&
       bus_type != V4L2_MBUS_CSI2_CPHY)
    {
        v4l2_fwnode_endpoint_free(&v4l2_ep);
        dev_err(priv->dev, "Unsupport bus-type %u on port %u\n",
                bus_type, pad);
        return -EINVAL;
    }

    ret = 0;
    if(v4l2_ep.nr_of_link_frequencies == 0)
        link_frequency = MAX_DES_LINK_FREQUENCY_DEFAULT;
    else if(v4l2_ep.nr_of_link_frequencies == 1)
        link_frequency = v4l2_ep.nr_of_link_frequencies[0];
    else:
        ret = -EINVAL;

    v4l2_fwnode_endpoint_free(&v4l2_ep);

    if(ret)
    {
        dev_err(priv->dev, "Invalid number of link frequencies %u on port %u\n",
                v4l2_ep.nr_of_link_frequencies, pad);
        return -EINVAL;
    }


    if(link_frequency < MAX_DES_LINK_FREQUENCY_MIN ||
       link_frequency > MAX_DES_LINK_FREQUENCY_MAX  )
    {
        dev_err(priv->dev, "Invalid link frequency %u on port %u\n",
                link_frequency, pad);
        return -EINVAL;
    }

    for(i = 0; i < mipi->num_data_lanes; i++)
    {
        if(mipi->data_lanes[i] > mipi->data_lanes){
            dev_err(priv->dev, "Invalid data lane %u on port %u\n",
                    mipi->data_lanes[i], pad);
        }
        return -EINVAL;
    }

    phy->bus_type = bus_type;
    phy->mipi = *mipi;
    phy->link_frequency = link_frequency;
    phy->enabled = true;
    
    return 0;
}

static int max_dex_find_phys_config(struct max_des_priv *priv)
{

}

int max_des_parse_dt(struct max_des_priv *priv)
{

    // struct device *dev = priv->dev;
    struct fwnode_handle *fwnode = dev_fwnode(priv->dev);
    struct max_des *des = priv->des; 
    struct max_des_phy *phy;
    struct max_des_pipe *pipe;
    struct max_des_link *link;
    unsigned int i;
    int ret;
    

    for(i=0;i<des->ops->num_phys;i++)
    {
        phy = &des->phys[i];
        phy->index = i;

        ret = max_des_parse_src_dt(priv, phy, fwnode);
        if(ret)
            return ret;
    }
    /* need to be create: used to find which phy's setting is match to DT.*/
    ret = max_des_find_phys_config(priv);
    if(ret)
        return ret;

    /*Find an */
    for(i=0; i< des->ops->num_phys; i++)
    {
        phy = &des->phys[i];

        if(!phy->enabled)
            priv->unused_phy = phy;
            break;
    }

    for()
    
    

    return 0;

}

unsigned int max_des_num_pads(struct max_des *des)
{
    return des->ops->num_links+des->ops->num_phys;
}


int max_des_allocate(struct max_des_priv *priv)
{

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
int max_des_probe(struct i2c_client *client, struct max_des *des)
{

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


int max_des_remove()
{
    return 0;
}