/* Copyright (c) 2024, Packet Sampling Extension
 * All rights reserved.
 *
 * Utility program to install flow entries with packet sampling actions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "../lib/ofp.h"
#include "../lib/ofpbuf.h"
#include "../lib/flow.h"
#include "../lib/vconn.h"
#include "../lib/vlog.h"

#define DEFAULT_PRIORITY 1000

static void usage(const char *progname) {
    printf("Usage: %s [OPTIONS] <switch-host:port>\n", progname);
    printf("Install flow entries with packet sampling\n\n");
    printf("Options:\n");
    printf("  -t, --table=TID         Table ID (default: 0)\n");
    printf("  -p, --priority=PRI      Flow priority (default: %d)\n", DEFAULT_PRIORITY);
    printf("  -i, --in-port=PORT      Match input port\n");
    printf("  -s, --src-ip=IP         Match source IP address\n");
    printf("  -d, --dst-ip=IP         Match destination IP address\n");
    printf("  --src-mac=MAC           Match source MAC address\n");
    printf("  --dst-mac=MAC           Match destination MAC address\n");
    printf("  --eth-type=TYPE         Match Ethernet type (hex)\n");
    printf("  --ip-proto=PROTO        Match IP protocol (number)\n");
    printf("  --sample-prob=PROB      Sampling probability (0-1000000, default: 1000000=100%%)\n");
    printf("  --max-rate=RATE         Maximum sampling rate (packets/sec, default: 1000)\n");
    printf("  --server-ip=IP          Receiver server IP address\n");
    printf("  --server-port=PORT      Receiver server port (default: 9999)\n");
    printf("  -o, --output=PORT       Output port for forwarding\n");
    printf("  -a, --add               Add flow (default)\n");
    printf("  -D, --delete            Delete matching flows\n");
    printf("  -h, --help              Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s tcp:192.168.1.1:6633 -i 1 -o 2 --sample-prob 500000 --max-rate 100 \\\n", progname);
    printf("     --server-ip 192.168.1.100 --server-port 9999\n");
    printf("  %s tcp:192.168.1.1:6633 -s 10.0.0.1 -d 10.0.0.2 --sample-prob 1000000\\\n", progname);
    printf("     --max-rate 10 --server-ip 192.168.1.100\n");
}

static int parse_mac_address(const char *str, uint8_t *mac) {
    return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6;
}

static int parse_ip_address(const char *str, uint32_t *ip) {
    struct in_addr addr;
    if (inet_pton(AF_INET, str, &addr) != 1) {
        return 0;
    }
    *ip = ntohl(addr.s_addr);
    return 1;
}

int main(int argc, char *argv[]) {
    struct vconn *vconn;
    struct ofpbuf *buffer;
    struct flow flow;
    int ret;
    
    /* Default parameters */
    uint8_t table_id = 0;
    uint16_t priority = DEFAULT_PRIORITY;
    uint32_t in_port = OFPP_ANY;
    uint32_t output_port = OFPP_ANY;
    uint32_t sample_probability = 1000000; /* 100% */
    uint32_t max_rate = 1000;
    uint32_t server_ip = 0;
    uint16_t server_port = 9999;
    int command = OFPFC_ADD;
    
    /* Match fields */
    uint8_t eth_src[6] = {0};
    uint8_t eth_dst[6] = {0};
    uint16_t eth_type = 0;
    uint32_t src_ip = 0;
    uint32_t dst_ip = 0;
    uint8_t ip_proto = 0;
    
    static struct option long_options[] = {
        {"table", required_argument, 0, 't'},
        {"priority", required_argument, 0, 'p'},
        {"in-port", required_argument, 0, 'i'},
        {"src-ip", required_argument, 0, 's'},
        {"dst-ip", required_argument, 0, 'd'},
        {"src-mac", required_argument, 0, 0},
        {"dst-mac", required_argument, 0, 0},
        {"eth-type", required_argument, 0, 0},
        {"ip-proto", required_argument, 0, 0},
        {"sample-prob", required_argument, 0, 0},
        {"max-rate", required_argument, 0, 0},
        {"server-ip", required_argument, 0, 0},
        {"server-port", required_argument, 0, 0},
        {"output", required_argument, 0, 'o'},
        {"add", no_argument, 0, 'a'},
        {"delete", no_argument, 0, 'D'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt, option_index;
    while ((opt = getopt_long(argc, argv, "t:p:i:s:d:o:aDh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 't':
                table_id = atoi(optarg);
                break;
            case 'p':
                priority = atoi(optarg);
                break;
            case 'i':
                in_port = atoi(optarg);
                break;
            case 's':
                if (!parse_ip_address(optarg, &src_ip)) {
                    fprintf(stderr, "Invalid source IP: %s\n", optarg);
                    return 1;
                }
                break;
            case 'd':
                if (!parse_ip_address(optarg, &dst_ip)) {
                    fprintf(stderr, "Invalid destination IP: %s\n", optarg);
                    return 1;
                }
                break;
            case 'o':
                output_port = atoi(optarg);
                break;
            case 'a':
                command = OFPFC_ADD;
                break;
            case 'D':
                command = OFPFC_DELETE;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case 0:
                if (strcmp(long_options[option_index].name, "src-mac") == 0) {
                    if (!parse_mac_address(optarg, eth_src)) {
                        fprintf(stderr, "Invalid source MAC: %s\n", optarg);
                        return 1;
                    }
                } else if (strcmp(long_options[option_index].name, "dst-mac") == 0) {
                    if (!parse_mac_address(optarg, eth_dst)) {
                        fprintf(stderr, "Invalid destination MAC: %s\n", optarg);
                        return 1;
                    }
                } else if (strcmp(long_options[option_index].name, "eth-type") == 0) {
                    eth_type = strtol(optarg, NULL, 16);
                } else if (strcmp(long_options[option_index].name, "ip-proto") == 0) {
                    ip_proto = atoi(optarg);
                } else if (strcmp(long_options[option_index].name, "sample-prob") == 0) {
                    sample_probability = atoi(optarg);
                    if (sample_probability > 1000000) {
                        fprintf(stderr, "Sampling probability must be <= 1000000\n");
                        return 1;
                    }
                } else if (strcmp(long_options[option_index].name, "max-rate") == 0) {
                    max_rate = atoi(optarg);
                } else if (strcmp(long_options[option_index].name, "server-ip") == 0) {
                    if (!parse_ip_address(optarg, &server_ip)) {
                        fprintf(stderr, "Invalid server IP: %s\n", optarg);
                        return 1;
                    }
                } else if (strcmp(long_options[option_index].name, "server-port") == 0) {
                    server_port = atoi(optarg);
                }
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Error: switch host:port required\n");
        usage(argv[0]);
        return 1;
    }
    
    if (command == OFPFC_ADD && server_ip == 0) {
        fprintf(stderr, "Error: server IP is required for sampling\n");
        return 1;
    }
    
    /* Initialize flow structure */
    memset(&flow, 0, sizeof(flow));
    
    /* Set match fields */
    if (in_port != OFPP_ANY) {
        flow.in_port = in_port;
    }
    
    if (eth_type != 0) {
        flow.dl_type = htons(eth_type);
    }
    
    if (src_ip != 0) {
        flow.nw_src = htonl(src_ip);
    }
    
    if (dst_ip != 0) {
        flow.nw_dst = htonl(dst_ip);
    }
    
    if (ip_proto != 0) {
        flow.nw_proto = ip_proto;
    }
    
    if (memcmp(eth_src, "\0\0\0\0\0\0", 6) != 0) {
        memcpy(flow.dl_src, eth_src, 6);
    }
    
    if (memcmp(eth_dst, "\0\0\0\0\0\0", 6) != 0) {
        memcpy(flow.dl_dst, eth_dst, 6);
    }
    
    /* Connect to switch */
    ret = vconn_open_block(argv[optind], OFP13_VERSION, &vconn);
    if (ret) {
        fprintf(stderr, "Failed to connect to switch: %s\n", argv[optind]);
        return 1;
    }
    
    /* Create flow mod message */
    if (command == OFPFC_ADD) {
        /* Calculate action length: output + sample */
        size_t actions_len = 0;
        
        if (output_port != OFPP_ANY) {
            actions_len += 16; /* output action */
        }
        
        actions_len += 24; /* sample action */
        
        buffer = make_flow_mod(command, table_id, &flow, actions_len);
        
        /* Add actions */
        struct ofp_action_header *actions = (struct ofp_action_header *)(buffer->data + sizeof(struct ofp_flow_mod));
        size_t action_offset = 0;
        
        if (output_port != OFPP_ANY) {
            struct ofp_action_output *output_act = (struct ofp_action_output *)&actions[action_offset];
            output_act->type = htons(OFPAT_OUTPUT);
            output_act->len = htons(16);
            output_act->port = htonl(output_port);
            output_act->max_len = htons(OFPCMTL_NO_BUFFER);
            memset(output_act->pad, 0, 6);
            action_offset += 16 / 8; /* 16 bytes = 2 * 8-byte units */
        }
        
        /* Add sample action */
        struct ofp_action_sample *sample_act = (struct ofp_action_sample *)&actions[action_offset];
        sample_act->type = htons(OFPAT_SAMPLE);
        sample_act->len = htons(24);
        sample_act->sample_probability = htonl(sample_probability);
        sample_act->max_rate = htonl(max_rate);
        sample_act->server_ip = htonl(server_ip);
        sample_act->server_port = htons(server_port);
        memset(sample_act->pad, 0, 6);
        
        printf("Installing flow entry:\n");
    } else {
        buffer = make_del_flow(&flow, table_id);
        printf("Deleting flow entries:\n");
    }
    
    /* Set priority */
    struct ofp_flow_mod *fm = buffer->data;
    fm->priority = htons(priority);
    
    /* Print flow information */
    if (in_port != OFPP_ANY) printf("  In Port: %u\n", in_port);
    if (src_ip != 0) printf("  Source IP: %u.%u.%u.%u\n", 
                           (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
                           (src_ip >> 8) & 0xFF, src_ip & 0xFF);
    if (dst_ip != 0) printf("  Dest IP: %u.%u.%u.%u\n",
                           (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
                           (dst_ip >> 8) & 0xFF, dst_ip & 0xFF);
    if (eth_type != 0) printf("  Ethernet Type: 0x%04x\n", eth_type);
    if (ip_proto != 0) printf("  IP Protocol: %u\n", ip_proto);
    
    if (command == OFPFC_ADD) {
        printf("  Sampling Probability: %u/%u\n", sample_probability, 1000000);
        printf("  Max Sampling Rate: %u pps\n", max_rate);
        printf("  Receiver Server: %u.%u.%u.%u:%u\n",
               (server_ip >> 24) & 0xFF, (server_ip >> 16) & 0xFF,
               (server_ip >> 8) & 0xFF, server_ip & 0xFF, server_port);
        if (output_port != OFPP_ANY) printf("  Output Port: %u\n", output_port);
    }
    
    printf("  Table ID: %u\n", table_id);
    printf("  Priority: %u\n", priority);
    
    /* Send message to switch */
    ret = vconn_send_block(vconn, buffer);
    if (ret) {
        fprintf(stderr, "Failed to send flow mod message\n");
        ofpbuf_delete(buffer);
        vconn_close(vconn);
        return 1;
    }
    
    printf("Flow mod message sent successfully\n");
    
    ofpbuf_delete(buffer);
    vconn_close(vconn);
    
    return 0;
}