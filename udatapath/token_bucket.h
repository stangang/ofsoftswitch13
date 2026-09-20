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

#ifndef TOKEN_BUCKET_H
#define TOKEN_BUCKET_H 1

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "timeval.h"

/****************************************************************************
 * Token bucket structure for rate limiting packet sampling
 ****************************************************************************/

struct token_bucket {
    uint32_t max_rate;          /* Maximum tokens per second */
    uint32_t tokens;            /* Current number of tokens */
    struct timeval last_update; /* Last time tokens were updated */
    bool enabled;               /* Whether this bucket is enabled */
};

/* Creates a new token bucket with the specified maximum rate */
struct token_bucket *
token_bucket_create(uint32_t max_rate);

/* Destroys a token bucket */
void
token_bucket_destroy(struct token_bucket *bucket);

/* Updates the token count based on elapsed time */
void
token_bucket_update(struct token_bucket *bucket);

/* Attempts to consume a token, returns true if successful */
bool
token_bucket_consume(struct token_bucket *bucket);

/* Resets the token bucket to its initial state */
void
token_bucket_reset(struct token_bucket *bucket);

/* Enables or disables the token bucket */
void
token_bucket_set_enabled(struct token_bucket *bucket, bool enabled);

#endif /* TOKEN_BUCKET_H */