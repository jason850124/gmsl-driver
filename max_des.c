#include <linux/delay.h>
#include <linux/i2c-atr.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <media/mipi-csi2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>


#include "max_des.h"
#include "max_serdes.h"

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
                                struct fwnode_handle *fwnode) //represent gmsl device self
{
    /*create fwnode_handle item as an endpoint, and anaylze it's format 
    and regist it to v4l2 subsystem ---- for phys (gmsl link to SoC)*/
    struct max_des *des = priv->des;
    struct v4l2_fwnode_endpoint v4l2_ep = {.bus = V4L2_MBUS_UNKNOWN};
    struct v4l2_mbus_config_mipi_csi2 *mipi = &v4l2_ep.bus.mipi_csi2;
    enum v4l2_mbus_type bus_type;
    struct fwnode_handle *ep;
    u64 link_frequency;
    unsigned int i;
    int ret;
    //get pad number
    u32 pad = max_des_phy_to_pad(des,phy);
    //get endpoint by port and endpoint numbers ---> (parent fwnode/ id for port/ id for ep under the port/ fwnode lookup flags)
    ep = fwnode_graph_get_endpoint_by_id(fwnode, pad, 0,0);
    if(!ep)
        return 0;
    //analyze endpoint and regist it to v4l2 sub-system
    ret = v4l2_fwnode_endpoint_alloc_parse(ep, &v4l2_ep);
    //release endpoint
    fwnode_handle_put(ep);
    if(ret)
    {
        dev_err(priv->dev, "Could not parse endpoint on port %u\n", pad);
        return ret;
    }
    //check interface type
    bus_type = v4l2_ep.bus_type;
    if(bus_type != V4L2_MBUS_CSI2_DPHY &&
       bus_type != V4L2_MBUS_CSI2_CPHY)
    {
        v4l2_fwnode_endpoint_free(&v4l2_ep);
        dev_err(priv->dev, "Unsupport bus-type %u on port %u\n",
                bus_type, pad);
        return -EINVAL;
    }
    //check frequency
    ret = 0;
    if(v4l2_ep.nr_of_link_frequencies == 0)
        link_frequency = MAX_DES_LINK_FREQUENCY_DEFAULT;
    else if(v4l2_ep.nr_of_link_frequencies == 1)
        link_frequency = v4l2_ep.nr_of_link_frequencies[0];
    else:
        ret = -EINVAL;
    //release v4l2 endpoint
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

    //check mipi's number of lanes (mipi is endpoint's physical)
    for(i = 0; i < mipi->num_data_lanes; i++)
    {
        if(mipi->data_lanes[i] > mipi->data_lanes){
            dev_err(priv->dev, "Invalid data lane %u on port %u\n",
                    mipi->data_lanes[i], pad);
        }
        return -EINVAL;
    }

    //load features to device's settings
    phy->bus_type = bus_type;
    phy->mipi = *mipi;
    phy->link_frequency = link_frequency;
    phy->enabled = true;
    
    return 0;
}

static int max_dex_find_phys_config(struct max_des_priv *priv)
{   
    /*use this function to check DT's phys configuration 
      is under device's limit and match one of device's settings.*/
    struct max_des *des = priv->des;
    const struct max_phys_configs *configs = &des->ops->phys_configs;
    struct max_des_phy *phy;
    
    int i,j;

    for(i = 0; i < configs->num_configs; i++)
    {
        const struct max_phys_config *config = &configs->configs[i];
        bool matching = true;
        bool all_phys_disable = true;

        for(j = 0; j < des->ops->num_phys; j++)
        {
            phy = &des->phys[j];

            if(!phy->enabled)
                continue;

            all_phys_disable = false;

            if(phy->mipi.data_lanes <= config->lanes[j] &&
               phy->mipi.clock_lane == config->clock_lane[j])
                continue;

            matching = false;
            break;
        }

        if(matching)
            break;

    }

    //identify the exact error
    if(i == configs->num_configs)
    {
        if(all_phys_disable)
            dev_err(priv->dev, "All Phys don't enable.\n");
        else
            dev_err(priv->dev, "Invalid lane configuration.\n");
        return -EINVAL;
    }

    des->phys_config = i;

    return 0;

}

static int max_des_find_link_source(struct max_des_priv *priv, struct max_des_link *link)
{
    return &priv->sources[link->index];
}

static int max_des_link_to_pad(struct max_des *des, struct max_des_link *link)
{
    return link->index;
}
//we need to check whole error codes' means
static int max_des_parse_sink_dt(struct max_des_priv *priv, struct max_des_link *link,
                                 struct max_source *source, struct fwnode_handle *fwnode)
{
    struct max_des *des = priv->des;
    struct fwnode_handle *ep;
    char poc_name[10];
    int ret;
    // unsigned int index = link->index;

    //get pad number
    u32 pad = max_des_link_to_pad(des,link);
    //get endpoint by port and endpoint numbers ---> (parent fwnode/ id for port/ id for ep under the port/ fwnode lookup flags)
    ep = fwnode_graph_get_endpoint_by_id(fwnode, pad, 0,0);
    if(!ep)
        return 0;

    source->ep_fwnode = fwnode_graph_get_remote_endpoint(ep);
    fwnode_handle_put(ep);
    if(!source->ep_fwnode)
    {
        dev_err(priv->dev, "Failed to get remote endpoint on port %u\n", pad);
        return -ENODEV;
    }

    snprintf(poc_name, sizeof(poc_name), "port%u-poc", pad); 
    /*This function is providing an optional power supply controller to Linux regulator framework*/
    priv->pocs[pad] = devm_regulator_get_optional(priv->dev, poc_name);
    /*IS_ERR: check if member is an error flag*/
    if(IS_ERR(priv->pocs[pad]))
    {   
        /*get back error code*/
        ret = PTR_ERR(priv->pocs[pad]);
        if(ret != -ENODEV)
        {
            dev_err(priv->dev, "Failed to get POC supply on part %u: %d\n", pad, ret);
            goto err_put_source_ep_fwnodee;
        }

        priv->pocs[pad] = NULL;
    }

    link->enabled = true;

    err_put_source_ep_fwnode:
        fwnode_handle_put(source->ep_fwnode);

    return ret;
    
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
    
    /*1. checkout and set phys*/
    for(i = 0; i < des->ops->num_phys; i++)
    {
        phy = &des->phys[i];
        phy->index = i;

        ret = max_des_parse_src_dt(priv, phy, fwnode);
        if(ret)
            return ret;
    }
    /*find which phy's setting is match to DT.*/
    ret = max_des_find_phys_config(priv);
    if(ret)
        return ret;

    /*Find an unused PHY to send unampped data to.*/
    for(i = 0; i < des->ops->num_phys; i++)
    {
        phy = &des->phys[i];

        if(!phy->enabled)
            priv->unused_phy = phy;
            break;
    }


    /*2. checkout and set pipes (should checkout stream id also)*/
    for(i = 0; i < des->ops->num_pipes; i++)
    {
        pipe = &des->pipes[i];

        if(des->ops->needs_unique_stream_id)
            pipe->stream_id = i;
        else
            pipe->stream_id = 0;

        pipe->link_id = i;
    }

    /*3. checkout and set links*/
    for(i = 0; i < des->ops->num_links; i++)
    {
        struct max_source *source;
        link = &des->links[i];
        link->index = i;

        source = max_des_find_link_source(priv, link);
        if(!source)
            return -ENOENT;
        
        source->index = i;

        ret = max_des_parse_sink_dt(priv, link, source, fwnode);
        if(ret)
            return ret;

    }
    

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

static int max_des_set_pipe_remaps(struct max_des_priv *priv, struct max_des_pipe *pipe,
                                   struct max_des_remap *remaps, unsigned int num_remaps)
{
    //need to check2
    struct max_des *des = priv->des;
    unsigned int mask = 0;
    unsigned int i;
    int ret;

    for(i = 0; i < num_remaps; i++)
    {
        struct max_des_remap *remap = &remaps[i];

        ret = des->ops->set_pipe_remap(des, pipe, i, remap);
        if(ret)
            return ret;

        mask |= BIT(i);
    }

    return des->ops->set_pipe_remaps_enable(des, pipe, mask);

}

static int max_des_init(struct max_des_priv *priv)
{
    struct max_des *des = priv->des;
    unsigned int i;
    int ret;

    /*we can check if device has init process or default settings*/
    if(des->ops->init)
    {
        ret = des->ops->init(des);
        if(ret)
            return ret;
    }
    
    /*disable device until whole settings are ready*/
    ret = des->ops->set_enable(des,false);
    if(ret)
        return ret;

    for(i = 0; i < des->ops->num_phys; i++)
    {
        struct max_des_phy *phy = &des->phys[i];

        if(phy->enabled)
        {
            ret = des->ops->init_phy(des, phy);
            if(ret)
                return ret;
        }   

        ret = des->ops->set_phy_active(des, phy, false);
        if(ret)
            return ret;
    }
    /*need to check(why pipes is init like this)*/
    for(i = 0; i < des->ops->num_pipes; i++)
    {
        struct max_des_pipe *pipe = &des->pipes[i];

        ret = des->ops->set_pipe_enable(des, pipe, false);
        if(ret)
            return ret;
        
        if(des->ops->set_pipe_tunnel_enable)
        {
            ret = des->ops->set_pipe_tunnel_enable(des, pipe, false);
            if(ret)
                return ret;
        }

        ret = des->ops->set_pipe_stream_id(des, pipe, pipe->stream_id);
        if(ret)
            return ret;

        ret = max_des_set_pipe_remaps(priv, pipe, pipe->remaps, pipe->num_remaps);
        if(ret)
            return ret;

    }

    /*need to check*/
    if(!des->ops->init_link)
        return 0;

    for(i = 0; i < des->ops->num_links; i++)
    {
        struct max_des_link *link = &des->links[i];

        if(!link->enabled)
            continue;

        ret = des->ops->init_link(des, link);
        if(ret)
            return ret;
    }

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
    ret = max_des_init(priv);
    if(ret)
        return ret;

    //7. manage power function


    //8. i2c init

    //9. register v4l2 subdev




    return 0;
}


int max_des_remove()
{
    return 0;
}