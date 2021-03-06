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

#ifndef FLB_OUT_PULSAR_CONTEXT_H
#define FLB_OUT_PULSAR_CONTEXT_H

#include "pulsar_client.h"

struct flb_pulsar_context
{
    struct flb_pulsar_client *client;

    struct flb_output_instance *output_instance;

    void(*publish_fn) (struct flb_pulsar_context * context,
                                pulsar_message_t * msg);

    pulsar_result(*flush_fn) (struct flb_pulsar_context * context);

    pulsar_result(*connect_fn) (struct flb_pulsar_context * context);
};

struct flb_pulsar_context *flb_pulsar_context_create(struct
                                                     flb_output_instance *ins,
                                                     struct flb_config
                                                     *config);

int flb_pulsar_context_destroy(struct flb_pulsar_context *ctx);

#endif
