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
#include "token_bucket.h"
#include "timeval.h"
#include "util.h"

struct token_bucket *
token_bucket_create(uint32_t max_rate) {
    struct token_bucket *bucket = xmalloc(sizeof(struct token_bucket));
    
    bucket->max_rate = max_rate;
    bucket->tokens = max_rate; /* Start with full bucket */
    gettimeofday(&bucket->last_update, NULL);
    bucket->enabled = true;
    
    return bucket;
}

void
token_bucket_destroy(struct token_bucket *bucket) {
    free(bucket);
}

void
token_bucket_update(struct token_bucket *bucket) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    /* Calculate elapsed time in seconds */
    double elapsed = timeval_to_double(&now) - timeval_to_double(&bucket->last_update);
    
    if (elapsed > 0) {
        /* Add tokens based on elapsed time */
        uint32_t new_tokens = (uint32_t)(elapsed * bucket->max_rate);
        bucket->tokens += new_tokens;
        
        /* Cap tokens at maximum rate */
        if (bucket->tokens > bucket->max_rate) {
            bucket->tokens = bucket->max_rate;
        }
        
        bucket->last_update = now;
    }
}

bool
token_bucket_consume(struct token_bucket *bucket) {
    if (!bucket->enabled) {
        return true; /* If disabled, always allow consumption */
    }
    
    token_bucket_update(bucket);
    
    if (bucket->tokens > 0) {
        bucket->tokens--;
        return true;
    }
    
    return false;
}

void
token_bucket_reset(struct token_bucket *bucket) {
    bucket->tokens = bucket->max_rate;
    gettimeofday(&bucket->last_update, NULL);
}

void
token_bucket_set_enabled(struct token_bucket *bucket, bool enabled) {
    bucket->enabled = enabled;
}