/*
Copyright (C) 2026 Sergei Vostrikov
SPDX-License-Identifier: Apache-2.0
*/

#ifndef WULPUS_TCP_TRANSPORT_H
#define WULPUS_TCP_TRANSPORT_H

#include "sock.h"
#include "wulpus_transport.h"

esp_err_t wulpus_tcp_transport_create(wulpus_transport_t *transport,
                                      socket_instance_t *socket);

#endif
