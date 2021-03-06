/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019      The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef FLB_OUT_PULSAR_CLIENT_H
#define FLB_OUT_PULSAR_CLIENT_H

#include <fluent-bit/flb_output.h>
#include <fluent-bit/flb_config.h>

#include <pulsar/c/client.h>

struct flb_pulsar_client
{
    pulsar_client_t *client;
    pulsar_client_configuration_t *client_config;
    pulsar_producer_t *producer;
    pulsar_producer_configuration_t *producer_config;
};

struct flb_pulsar_client *flb_pulsar_client_create(struct flb_output_instance
                                                   *ins,
                                                   struct flb_config *config);

pulsar_result flb_pulsar_client_create_producer(struct flb_pulsar_client
                                                *client,
                                                struct flb_output_instance
                                                *ins);

void flb_pulsar_client_produce_message(struct flb_pulsar_client
                                                *client,
                                                pulsar_message_t * msg);

pulsar_result flb_pulsar_client_flush(struct flb_pulsar_client *
                                                client);

int flb_pulsar_client_destroy(struct flb_pulsar_client *client);

#endif
