/****************************************************************************
 * net/icmp/icmp_send.c
 *
 *   Copyright (C) 2008-2010, 2012, 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING)

#include <debug.h>

#include <arpa/inet.h>

#include <nuttx/net/netconfig.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/netstats.h>
#include <nuttx/net/ip.h>

#include "devif/devif.h"
#include "utils/utils.h"
#include "icmp/icmp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMPBUF ((struct icmp_iphdr_s *)&dev->d_buf[NET_LLH_LEN])

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: icmp_send
 *
 * Description:
 *   Setup to send an ICMP packet
 *
 * Parameters:
 *   dev - The device driver structure to use in the send operation
 *
 * Return:
 *   None
 *
 * Assumptions:
 *   Called from the interrupt level or with interrupts disabled.
 *
 ****************************************************************************/

void icmp_send(FAR struct net_driver_s *dev, FAR net_ipaddr_t *destaddr)
{
  FAR struct icmp_iphdr_s *picmp = ICMPBUF;

  if (dev->d_sndlen > 0)
    {
      /* The total length to send is the size of the application data plus
       * the IP and ICMP headers (and, eventually, the Ethernet header)
       */

      dev->d_len = dev->d_sndlen + UIP_IPICMPH_LEN;

      /* The total size of the data (for ICMP checksum calculation) includes
       * the size of the ICMP header
       */

      dev->d_sndlen += UIP_ICMPH_LEN;

      /* Initialize the IP header.  Note that for IPv6, the IP length field
       * does not include the IPv6 IP header length.
       */

#ifdef CONFIG_NET_IPv6

      picmp->vtc         = 0x60;
      picmp->tcf         = 0x00;
      picmp->flow        = 0x00;
      picmp->len[0]      = (dev->d_sndlen >> 8);
      picmp->len[1]      = (dev->d_sndlen & 0xff);
      picmp->nexthdr     = IP_PROTO_ICMP;
      picmp->hoplimit    = UIP_TTL;

      net_ipaddr_copy(picmp->srcipaddr, &dev->d_ipaddr);
      net_ipaddr_copy(picmp->destipaddr, destaddr);

#else /* CONFIG_NET_IPv6 */

      picmp->vhl         = 0x45;
      picmp->tos         = 0;
      picmp->len[0]      = (dev->d_len >> 8);
      picmp->len[1]      = (dev->d_len & 0xff);
      ++g_ipid;
      picmp->ipid[0]     = g_ipid >> 8;
      picmp->ipid[1]     = g_ipid & 0xff;
      picmp->ipoffset[0] = UIP_TCPFLAG_DONTFRAG >> 8;
      picmp->ipoffset[1] = UIP_TCPFLAG_DONTFRAG & 0xff;
      picmp->ttl         = UIP_TTL;
      picmp->proto       = IP_PROTO_ICMP;

      net_ipaddr_hdrcopy(picmp->srcipaddr, &dev->d_ipaddr);
      net_ipaddr_hdrcopy(picmp->destipaddr, destaddr);

      /* Calculate IP checksum. */

      picmp->ipchksum    = 0;
      picmp->ipchksum    = ~(ip_chksum(dev));

#endif /* CONFIG_NET_IPv6 */

      /* Calculate the ICMP checksum. */

      picmp->icmpchksum  = 0;
      picmp->icmpchksum  = ~(icmp_chksum(dev, dev->d_sndlen));
      if (picmp->icmpchksum == 0)
        {
          picmp->icmpchksum = 0xffff;
        }

      nllvdbg("Outgoing ICMP packet length: %d (%d)\n",
              dev->d_len, (picmp->len[0] << 8) | picmp->len[1]);

#ifdef CONFIG_NET_STATISTICS
      g_netstats.icmp.sent++;
      g_netstats.ip.sent++;
#endif
    }
}

#endif /* CONFIG_NET && CONFIG_NET_ICMP && CONFIG_NET_ICMP_PING */
