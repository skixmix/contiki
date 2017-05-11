/*
 * Copyright (c) 2016, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#ifndef __IOTLAB_PROJECT_CONF_H__
#define __IOTLAB_PROJECT_CONF_H__

#define NETWORK_SIZE 256
#define NETWORK_DENSITY 160

#ifndef IOTLAB_WITH_NON_STORING
#define IOTLAB_WITH_NON_STORING 1 /* Set this to run with RPL non-storing mode */
#endif

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS NETWORK_DENSITY

/* Use no DIO suppression */
#undef RPL_CONF_DIO_REDUNDANCY
#define RPL_CONF_DIO_REDUNDANCY 0xff

#if IOTLAB_WITH_NON_STORING

#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM NETWORK_SIZE /* Number of links maintained at the root. Can be set to 0 at non-root nodes. */
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 0 /* No need for routes */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/

#else /* IOTLAB_WITH_NON_STORING */

#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 0
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES  NETWORK_SIZE
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST

#endif /* IOTLAB_WITH_NON_STORING */

#endif /* __IOTLAB_PROJECT_CONF_H__ */
