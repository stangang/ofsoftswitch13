/* Copyright (c) 2024, Packet Sampling Extension
 * All rights reserved.
 *
 * Simple UDP packet receiver for sampled packets from OpenFlow switch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <pcap/pcap.h>

#define BUFFER_SIZE 65536
#define PCAP_FILENAME "sampled_packets.pcap"

static int running = 1;
static pcap_dumper_t *pcap_dumper = NULL;

void signal_handler(int sig) {
    running = 0;
    printf("\nShutting down packet receiver...\n");
}

void usage(const char *progname) {
    printf("Usage: %s <port> [pcap_file]\n", progname);
    printf("  port: UDP port to listen on\n");
    printf("  pcap_file: Optional pcap file to save packets (default: %s)\n", PCAP_FILENAME);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t buffer[BUFFER_SIZE];
    const char *pcap_filename = PCAP_FILENAME;
    
    if (argc < 2) {
        usage(argv[0]);
    }
    
    if (argc >= 3) {
        pcap_filename = argv[2];
    }
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return 1;
    }
    
    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return 1;
    }
    
    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind socket */
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }
    
    /* Create pcap file for saving packets */
    pcap_t *pcap = pcap_open_dead(DLT_EN10MB, 65535);
    if (!pcap) {
        fprintf(stderr, "Failed to create pcap handle\n");
        close(sockfd);
        return 1;
    }
    
    pcap_dumper = pcap_dump_open(pcap, pcap_filename);
    if (!pcap_dumper) {
        fprintf(stderr, "Failed to create pcap file: %s\n", pcap_filename);
        pcap_close(pcap);
        close(sockfd);
        return 1;
    }
    
    pcap_close(pcap);
    
    /* Set up signal handler for graceful shutdown */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Packet receiver started on port %d\n", port);
    printf("Saving packets to: %s\n", pcap_filename);
    printf("Press Ctrl+C to stop\n\n");
    
    uint64_t packet_count = 0;
    
    while (running) {
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                   (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            if (running) {
                perror("recvfrom failed");
            }
            continue;
        }
        
        if (recv_len < 12) {
            fprintf(stderr, "Received packet too small: %zd bytes\n", recv_len);
            continue;
        }
        
        /* Parse packet header */
        uint32_t timestamp_sec = ntohl(*(uint32_t*)&buffer[0]);
        uint32_t timestamp_usec = ntohl(*(uint32_t*)&buffer[4]);
        uint32_t in_port = ntohl(*(uint32_t*)&buffer[8]);
        uint16_t packet_length = ntohs(*(uint16_t*)&buffer[12]);
        
        if (recv_len != (14 + packet_length)) {
            fprintf(stderr, "Packet length mismatch: header=%u, actual=%zd\n", 
                    packet_length, recv_len - 14);
            continue;
        }
        
        packet_count++;
        
        /* Create pcap packet header */
        struct pcap_pkthdr pcap_header;
        struct timeval tv;
        tv.tv_sec = timestamp_sec;
        tv.tv_usec = timestamp_usec;
        
        pcap_header.ts = tv;
        pcap_header.caplen = packet_length;
        pcap_header.len = packet_length;
        
        /* Write to pcap file */
        pcap_dump((u_char *)pcap_dumper, &pcap_header, &buffer[14]);
        
        /* Print packet information */
        char timestamp_str[64];
        time_t ts = timestamp_sec;
        struct tm *tm_info = localtime(&ts);
        strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        printf("[%s.%06u] Packet #%lu: port=%u, length=%u bytes, from %s:%d\n",
               timestamp_str, timestamp_usec, packet_count, in_port, packet_length,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Flush pcap file periodically */
        if (packet_count % 100 == 0) {
            pcap_dump_flush(pcap_dumper);
        }
    }
    
    /* Cleanup */
    if (pcap_dumper) {
        pcap_dump_close(pcap_dumper);
    }
    
    close(sockfd);
    
    printf("\nReceived %lu packets total. Exiting.\n", packet_count);
    return 0;
}