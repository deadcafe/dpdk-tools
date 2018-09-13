#ifndef _BEEF_PORT_H_
#define _BEEF_PORT_H_

struct beef_port_s {
    MARKER cacheline0;
    char name[32];

    STAILQ_ENTRY(beef_port_s) node;

    uint16_t port_id;
    uint16_t queue_id;

    enum fpe_core_netdev_type_e netdev_type;
    uint16_t nb_slaves;
    uint16_t slaves[2]; /*!< slave port_id */

    const struct rte_port_in_ops *in;
    const struct rte_port_out_ops *out;
} __rte_cache_aligned;

#endif /* !_BEEF_PORT_H_ */
