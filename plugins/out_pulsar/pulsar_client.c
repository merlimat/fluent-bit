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

#include "./pulsar_client.h"
#include "./pulsar_config.h"

#include <math.h>

struct flb_pulsar_client *flb_pulsar_client_create(struct flb_output_instance
                                                   *ins,
                                                   struct flb_config *config)
{
    struct flb_pulsar_client *client =
        flb_calloc(1, sizeof(struct flb_pulsar_client));

    if (!client) {
        flb_errno();
        return NULL;
    }

    client->client_config = flb_pulsar_config_build_client_config(ins);
    client->producer_config = flb_pulsar_config_build_producer_config(ins);

    if (!client->client_config || !client->producer_config) {
        flb_error
            ("[out_pulsar] Unable to create pulsar configuration objects");
        flb_pulsar_client_destroy(client);
        return NULL;
    }

    // client init
    flb_output_net_default("localhost", 6650, ins);
    char *service_url =
        flb_malloc(13 + strlen(ins->host.name) +
                   ceil(log10(ins->host.port)) + 1);
    sprintf(service_url, "pulsar%s://%s:%d", (ins->use_tls ? "+ssl" : ""), ins->host.name, ins->host.port);

    client->client = pulsar_client_create(service_url, client->client_config);

    if (!client->client) {
        flb_error
            ("[out_pulsar] Unable to create client object for service URL %s",
             service_url);
        flb_pulsar_client_destroy(client);
        return NULL;
    }

    return client;
}

pulsar_result flb_pulsar_client_create_producer(struct flb_pulsar_client *
                                                client,
                                                struct flb_output_instance *
                                                ins)
{
    pulsar_result result = pulsar_result_UnknownError;
    char *property = NULL;
    char *topic = (property = flb_output_get_property("topic",
                                                      ins)) ? property :
        "fluent-bit";


    if (client && client->client && client->producer_config) {
        pulsar_result create_producer_result =
            pulsar_client_create_producer(client->client, topic,
                                          client->producer_config,
                                          &client->producer);
        if (create_producer_result != pulsar_result_Ok || !client->producer) {
            flb_error
                ("[out_pulsar] Failed to create producer for topic %s, result was %s (pointer is %s)",
                 topic, pulsar_result_str(create_producer_result),
                 client->producer ? "not null" : "NULL");
        }
        result = create_producer_result;
    }
    else {
        flb_error
            ("[out_pulsar] Unknown state; attempted to create producer with missing client and/or config objects.");
    }
    return result;
}

pulsar_result flb_pulsar_client_produce_message(struct flb_pulsar_client *
                                                client,
                                                pulsar_message_t * msg)
{
    return pulsar_producer_send(client->producer, msg);
}


int flb_pulsar_client_destroy(struct flb_pulsar_client *client)
{
    if (client) {
        if (client->producer) {
            pulsar_producer_close(client->producer);
            pulsar_producer_free(client->producer);
        }

        if (client->producer_config) {
            pulsar_producer_configuration_free(client->producer_config);
        }

        if (client->client) {
            pulsar_client_close(client->client);
            pulsar_client_free(client->client);
        }

        if (client->client_config) {
            pulsar_client_configuration_free(client->client_config);
        }
        flb_free(client);
    }

    return 0;
}
