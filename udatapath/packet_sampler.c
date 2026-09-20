/* Copyright (c) 2024, Packet Sampling Extension
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Packet Sampling Extension nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "packet_sampler.h"
#include "token_bucket.h"
#include "util.h"
#include "vlog.h"

#define LOG_MODULE VLM_packet_sampler

struct packet_sampler {
    int udp_socket;
};

struct packet_sampler *
packet_sampler_create(void) {
    struct packet_sampler *sampler = xmalloc(sizeof(struct packet_sampler));
    
    /* Create UDP socket for sending sampled packets */
    sampler->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sampler->udp_socket < 0) {
        VLOG_ERR(LOG_MODULE, "Failed to create UDP socket for packet sampling");
        free(sampler);
        return NULL;
    }
    
    return sampler;
}

void
packet_sampler_destroy(struct packet_sampler *sampler) {
    if (sampler) {
        if (sampler->udp_socket >= 0) {
            close(sampler->udp_socket);
        }
        free(sampler);
    }
}

bool
packet_sampler_sample_packet(struct packet_sampler *sampler, 
                            struct packet *pkt,
                            uint32_t sample_probability,
                            uint32_t max_rate,
                            uint32_t server_ip,
                            uint16_t server_port,
                            struct token_bucket *bucket) {
    
    /* Check sampling probability */
    if (sample_probability == 0) {
        return false; /* Sampling disabled */
    }
    
    /* Apply sampling probability */
    if (sample_probability < 1000000) {
        uint32_t random_value = random() % 1000000;
        if (random_value >= sample_probability) {
            return false; /* Not selected for sampling */
        }
    }
    
    /* Check rate limiting using token bucket */
    if (bucket && !token_bucket_consume(bucket)) {
        return false; /* Rate limit exceeded */
    }
    
    /* Send sampled packet to the receiver server */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(server_ip);
    server_addr.sin_port = htons(server_port);
    
    /* Create packet header with metadata */
    struct {
        uint32_t timestamp_sec;
        uint32_t timestamp_usec;
        uint32_t in_port;
        uint16_t packet_length;
        uint8_t  packet_data[0];
    } __attribute__((packed)) sampled_packet;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    sampled_packet.timestamp_sec = htonl(tv.tv_sec);
    sampled_packet.timestamp_usec = htonl(tv.tv_usec);
    sampled_packet.in_port = htonl(pkt->in_port);
    sampled_packet.packet_length = htons(pkt->buffer->size);
    
    /* Create buffer with header and packet data */
    size_t total_size = sizeof(sampled_packet) + pkt->buffer->size;
    uint8_t *send_buffer = xmalloc(total_size);
    
    memcpy(send_buffer, &sampled_packet, sizeof(sampled_packet));
    memcpy(send_buffer + sizeof(sampled_packet), pkt->buffer->data, pkt->buffer->size);
    
    /* Send the sampled packet */
    ssize_t sent = sendto(sampler->udp_socket, send_buffer, total_size, 0,
                         (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    free(send_buffer);
    
    if (sent < 0) {
        VLOG_WARN(LOG_MODULE, "Failed to send sampled packet to server");
        return false;
    }
    
    VLOG_DBG(LOG_MODULE, "Sampled packet sent to %s:%d", 
             inet_ntoa(*(struct in_addr*)&server_ip), server_port);
    
    return true;
}