/* packet-dcd.c
 * Routines for DCD Message dissection
 * Copyright 2004, Darryl Hymel <darryl.hymel[AT]arrisi.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>

void proto_register_docsis_dcd(void);
void proto_reg_handoff_docsis_dcd(void);

#define DCD_DOWN_CLASSIFIER 23
#define DCD_DSG_RULE 50
#define DCD_DSG_CONFIG 51

/* Define Downstrean Classifier subtypes
 * These are subtype of DCD_DOWN_CLASSIFIER (23)
 */

#define DCD_CFR_ID 2
#define DCD_CFR_RULE_PRI 5
#define DCD_CFR_IP_CLASSIFIER 9

/* Define IP Classifier sub-subtypes
 * These are subtypes of DCD_CFR_IP_CLASSIFIER (23.9)
 */
#define DCD_CFR_IP_SOURCE_ADDR 3
#define DCD_CFR_IP_SOURCE_MASK 4
#define DCD_CFR_IP_DEST_ADDR 5
#define DCD_CFR_IP_DEST_MASK 6
#define DCD_CFR_TCPUDP_SRCPORT_START 7
#define DCD_CFR_TCPUDP_SRCPORT_END 8
#define DCD_CFR_TCPUDP_DSTPORT_START 9
#define DCD_CFR_TCPUDP_DSTPORT_END 10

/* Define DSG Rule subtypes
 * These are subtype of DCD_DSG_RULE (50)
 */

#define DCD_RULE_ID 1
#define DCD_RULE_PRI 2
#define DCD_RULE_UCID_RNG 3
#define DCD_RULE_CLIENT_ID 4
#define DCD_RULE_TUNL_ADDR 5
#define DCD_RULE_CFR_ID 6
#define DCD_RULE_VENDOR_SPEC 43
/* Define DSG Rule Client ID sub-subtypes
 * These are subtypes of DCD_RULE_CLIENT_ID (50.4)
 */
#define DCD_CLID_BCAST_ID 1
#define DCD_CLID_KNOWN_MAC_ADDR 2
#define DCD_CLID_CA_SYS_ID 3
#define DCD_CLID_APP_ID 4

/* Define DSG Configuration subtypes
 * These are subtype of DCD_DSG_CONFIG (51)
 */

#define DCD_CFG_CHAN_LST 1
#define DCD_CFG_TDSG1 2
#define DCD_CFG_TDSG2 3
#define DCD_CFG_TDSG3 4
#define DCD_CFG_TDSG4 5
#define DCD_CFG_VENDOR_SPEC 43

/* Initialize the protocol and registered fields */
static int proto_docsis_dcd = -1;

static int hf_docsis_dcd_config_ch_cnt = -1;
static int hf_docsis_dcd_num_of_frag = -1;
static int hf_docsis_dcd_frag_sequence_num = -1;
static int hf_docsis_dcd_type = -1;
static int hf_docsis_dcd_length = -1;
static int hf_docsis_dcd_down_classifier_subtype = -1;
static int hf_docsis_dcd_down_classifier_length = -1;
static int hf_docsis_dcd_cfr_id = -1;
static int hf_docsis_dcd_cfr_rule_pri = -1;
static int hf_docsis_dcd_cfr_ip_subtype = -1;
static int hf_docsis_dcd_cfr_ip_length = -1;
static int hf_docsis_dcd_cfr_ip_source_addr = -1;
static int hf_docsis_dcd_cfr_ip_source_mask = -1;
static int hf_docsis_dcd_cfr_ip_dest_addr = -1;
static int hf_docsis_dcd_cfr_ip_dest_mask = -1;
static int hf_docsis_dcd_cfr_tcpudp_srcport_start = -1;
static int hf_docsis_dcd_cfr_tcpudp_srcport_end = -1;
static int hf_docsis_dcd_cfr_tcpudp_dstport_start = -1;
static int hf_docsis_dcd_cfr_tcpudp_dstport_end = -1;
static int hf_docsis_dcd_rule_id = -1;
static int hf_docsis_dcd_rule_pri = -1;
static int hf_docsis_dcd_rule_ucid_list = -1;
static int hf_docsis_dcd_clid_subtype = -1;
static int hf_docsis_dcd_clid_length = -1;
static int hf_docsis_dcd_clid_bcast_id = -1;
static int hf_docsis_dcd_clid_known_mac_addr = -1;
static int hf_docsis_dcd_clid_ca_sys_id = -1;
static int hf_docsis_dcd_clid_app_id = -1;
static int hf_docsis_dcd_dsg_rule_subtype = -1;
static int hf_docsis_dcd_dsg_rule_length = -1;
static int hf_docsis_dcd_rule_tunl_addr = -1;
static int hf_docsis_dcd_rule_cfr_id = -1;
static int hf_docsis_dcd_rule_vendor_spec = -1;
static int hf_docsis_dcd_cfg_subtype = -1;
static int hf_docsis_dcd_cfg_length = -1;
static int hf_docsis_dcd_cfg_chan = -1;
static int hf_docsis_dcd_cfg_tdsg1 = -1;
static int hf_docsis_dcd_cfg_tdsg2 = -1;
static int hf_docsis_dcd_cfg_tdsg3 = -1;
static int hf_docsis_dcd_cfg_tdsg4 = -1;
static int hf_docsis_dcd_cfg_vendor_spec = -1;

/* Initialize the subtree pointers */
static gint ett_docsis_dcd = -1;
static gint ett_docsis_dcd_cfr = -1;
static gint ett_docsis_dcd_cfr_ip = -1;
static gint ett_docsis_dcd_rule = -1;
static gint ett_docsis_dcd_clid = -1;
static gint ett_docsis_dcd_cfg = -1;
static gint ett_docsis_dcd_tlv = -1;

static const value_string dcd_tlv_vals[] = {
  {DCD_DOWN_CLASSIFIER, "DCD_CFR Encodings"},
  {DCD_DSG_RULE, "DCD DSG Rule Encodings"},
  {DCD_DSG_CONFIG, "DCD DSG Config Encodings"},
  {0, NULL}
};

static const value_string dcd_down_classifier_vals[] = {
  {DCD_CFR_ID, "Downstream Classifier Id"},
  {DCD_CFR_RULE_PRI, "Downstream Classifier Rule Priority"},
  {DCD_CFR_IP_CLASSIFIER, "DCD_CFR_IP Encodings"},
  {0, NULL}
};

static const value_string dcd_dsg_rule_vals[] = {
  {DCD_RULE_ID, "DSG Rule Id"},
  {DCD_RULE_PRI, "DSG Rule Priority"},
  {DCD_RULE_UCID_RNG, "DSG Rule UCID Range"},
  {DCD_RULE_CLIENT_ID, "DCD Rule ClientID Encodings"},
  {DCD_RULE_TUNL_ADDR, "DSG Rule Tunnel MAC Address"},
  {DCD_RULE_CFR_ID, "DSG Rule Classifier ID"},
  {DCD_RULE_VENDOR_SPEC, "DSG Rule Vendor Specific Parameters"},
  {0, NULL}
};

static const value_string dcd_clid_vals[] = {
  {DCD_CLID_BCAST_ID, "DSG Rule Client ID Broadcast ID"},
  {DCD_CLID_KNOWN_MAC_ADDR, "DSG Rule Client ID Known MAC Address"},
  {DCD_CLID_CA_SYS_ID, "DSG Rule Client ID CA System ID"},
  {DCD_CLID_APP_ID, "DSG Rule Client ID Application ID"},
  {0, NULL}
};

static const value_string dcd_cfr_ip_vals[] = {
  {DCD_CFR_IP_SOURCE_ADDR, "Downstream Classifier IP Source Address"},
  {DCD_CFR_IP_SOURCE_MASK, "Downstream Classifier IP Source Mask"},
  {DCD_CFR_IP_DEST_ADDR, "Downstream Classifier IP Destination Address"},
  {DCD_CFR_IP_DEST_MASK, "Downstream Classifier IP Destination Mask"},
  {DCD_CFR_TCPUDP_SRCPORT_START, "Downstream Classifier IP TCP/UDP Source Port Start"},
  {DCD_CFR_TCPUDP_SRCPORT_END, "Downstream Classifier IP TCP/UDP Source Port End"},
  {DCD_CFR_TCPUDP_DSTPORT_START, "Downstream Classifier IP TCP/UDP Destination Port Start"},
  {DCD_CFR_TCPUDP_DSTPORT_END, "Downstream Classifier IP TCP/UDP Destination Port End"},
  {0, NULL}
};

static const value_string dcd_cfg_vals[] = {
  {DCD_CFG_CHAN_LST, "DSG Configuration Channel"},
  {DCD_CFG_TDSG1, "DSG Initialization Timeout (Tdsg1)"},
  {DCD_CFG_TDSG2, "DSG Initialization Timeout (Tdsg2)"},
  {DCD_CFG_TDSG3, "DSG Initialization Timeout (Tdsg3)"},
  {DCD_CFG_TDSG4, "DSG Initialization Timeout (Tdsg4)"},
  {DCD_CFG_VENDOR_SPEC, "DSG Configuration Vendor Specific Parameters"},
  {0, NULL}
};

static expert_field ei_docsis_dcd_tlvlen_bad = EI_INIT;

static dissector_handle_t docsis_dcd_handle;

/* Dissection */
static void
dissect_dcd_dsg_cfg (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree, int start, guint16 len)
{
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree;
  proto_tree *dcd_item, *tlv_len_item;
  int pos;

  pos = start;
  while ( pos < ( start + len) )
  {
    type = tvb_get_guint8 (tvb, pos);
    dcd_tree = proto_tree_add_subtree(tree, tvb, pos, -1,
                                            ett_docsis_dcd_cfg, &dcd_item,
                                            val_to_str(type, dcd_cfg_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (dcd_tree, hf_docsis_dcd_cfg_subtype, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (dcd_tree, hf_docsis_dcd_cfg_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(dcd_item, length + 2);

    switch (type)
    {
    case DCD_CFG_CHAN_LST:
      if (length == 4)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_chan, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFG_TDSG1:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_tdsg1, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFG_TDSG2:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_tdsg2, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFG_TDSG3:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_tdsg3, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFG_TDSG4:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_tdsg4, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFG_VENDOR_SPEC:
      proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfg_vendor_spec, tvb, pos, length, ENC_NA);
      break;

    }

    pos += length;
  }
}

static void
dissect_dcd_down_classifier_ip (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree, int start, guint16 len)
{
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree;
  proto_tree *dcd_item, *tlv_len_item;
  int pos;

  pos = start;
  while ( pos < ( start + len) )
  {
    type = tvb_get_guint8 (tvb, pos);
    dcd_tree = proto_tree_add_subtree(tree, tvb, pos, -1,
                                            ett_docsis_dcd_cfr_ip, &dcd_item,
                                            val_to_str(type, dcd_cfr_ip_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (dcd_tree, hf_docsis_dcd_cfr_ip_subtype, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (dcd_tree, hf_docsis_dcd_cfr_ip_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(dcd_item, length + 2);

    switch (type)
    {
    case DCD_CFR_IP_SOURCE_ADDR:
      if (length == 4)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_ip_source_addr, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_IP_SOURCE_MASK:
      if (length == 4)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_ip_source_mask, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_IP_DEST_ADDR:
      if (length == 4)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_ip_dest_addr, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_IP_DEST_MASK:
      if (length == 4)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_ip_dest_mask, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_TCPUDP_SRCPORT_START:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_tcpudp_srcport_start, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_TCPUDP_SRCPORT_END:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_tcpudp_srcport_end, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_TCPUDP_DSTPORT_START:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_tcpudp_dstport_start, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_TCPUDP_DSTPORT_END:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_tcpudp_dstport_end, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    }

    pos += length;
  }
}

static void
dissect_dcd_clid (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree, int start, guint16 len)
{
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree;
  proto_tree *dcd_item, *tlv_len_item;
  int pos;

  pos = start;
  while ( pos < ( start + len) )
  {
    type = tvb_get_guint8 (tvb, pos);
    dcd_tree = proto_tree_add_subtree(tree, tvb, pos, -1,
                                            ett_docsis_dcd_clid, &dcd_item,
                                            val_to_str(type, dcd_clid_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (dcd_tree, hf_docsis_dcd_clid_subtype, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (dcd_tree, hf_docsis_dcd_clid_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(dcd_item, length + 2);

    switch (type)
    {
    case DCD_CLID_BCAST_ID:
      if (length == 2)
      {
        proto_tree_add_item(dcd_tree, hf_docsis_dcd_clid_bcast_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CLID_KNOWN_MAC_ADDR:
      if (length == 6)
      {
       proto_tree_add_item (dcd_tree, hf_docsis_dcd_clid_known_mac_addr, tvb, pos, length, ENC_NA);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CLID_CA_SYS_ID:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_clid_ca_sys_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CLID_APP_ID:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_clid_app_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    }

    pos += length;
  }
}

static void
dissect_dcd_dsg_rule (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree, int start, guint16 len)
{
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree;
  proto_tree *dcd_item, *tlv_len_item;
  int pos;

  pos = start;
  while ( pos < ( start + len) )
  {
    type = tvb_get_guint8 (tvb, pos);
    dcd_tree = proto_tree_add_subtree(tree, tvb, pos, -1,
                                            ett_docsis_dcd_rule, &dcd_item,
                                            val_to_str(type, dcd_dsg_rule_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (dcd_tree, hf_docsis_dcd_dsg_rule_subtype, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (dcd_tree, hf_docsis_dcd_dsg_rule_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(dcd_item, length + 2);

    switch (type)
    {
    case DCD_RULE_ID:
      if (length == 1)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_RULE_PRI:
      if (length == 1)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_pri, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_RULE_UCID_RNG:
      proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_ucid_list, tvb, pos, length, ENC_NA);
      break;
    case DCD_RULE_CLIENT_ID:
      dissect_dcd_clid (tvb, pinfo, dcd_tree, pos, length );
      break;
    case DCD_RULE_TUNL_ADDR:
      if (length == 6)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_tunl_addr, tvb, pos, length, ENC_NA);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_RULE_CFR_ID:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_cfr_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_RULE_VENDOR_SPEC:
      proto_tree_add_item (dcd_tree, hf_docsis_dcd_rule_vendor_spec, tvb, pos, length, ENC_NA);
      break;

    }

    pos += length;
  }
}

static void
dissect_dcd_down_classifier (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree, int start, guint16 len)
{
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree;
  proto_tree *dcd_item, *tlv_len_item;
  int pos;

  pos = start;
  while ( pos < ( start + len) )
  {
    type = tvb_get_guint8 (tvb, pos);
    dcd_tree = proto_tree_add_subtree(tree, tvb, pos, -1,
                                            ett_docsis_dcd_cfr, &dcd_item,
                                            val_to_str(type, dcd_down_classifier_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (dcd_tree, hf_docsis_dcd_down_classifier_subtype, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (dcd_tree, hf_docsis_dcd_down_classifier_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(dcd_item, length + 2);

    switch (type)
    {
    case DCD_CFR_ID:
      if (length == 2)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_id, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_RULE_PRI:
      if (length == 1)
      {
        proto_tree_add_item (dcd_tree, hf_docsis_dcd_cfr_rule_pri, tvb, pos, length, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_dcd_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    case DCD_CFR_IP_CLASSIFIER:
      dissect_dcd_down_classifier_ip (tvb , pinfo , dcd_tree , pos , length );
      break;
    }

    pos += length;
  }
}

static int
dissect_dcd (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  guint16 pos;
  guint8 type;
  guint32 length;
  proto_tree *dcd_tree, *tlv_tree;
  proto_item *dcd_item, *tlv_item;

  col_set_str(pinfo->cinfo, COL_INFO, "DCD Message: ");

  dcd_item = proto_tree_add_item(tree, proto_docsis_dcd, tvb, 0, -1, ENC_NA);
  dcd_tree = proto_item_add_subtree (dcd_item, ett_docsis_dcd);
  proto_tree_add_item (dcd_tree, hf_docsis_dcd_config_ch_cnt, tvb, 0, 1, ENC_BIG_ENDIAN);
  proto_tree_add_item (dcd_tree, hf_docsis_dcd_num_of_frag, tvb, 1, 1, ENC_BIG_ENDIAN);
  proto_tree_add_item (dcd_tree, hf_docsis_dcd_frag_sequence_num, tvb, 2, 1, ENC_BIG_ENDIAN);

  pos = 3;
  while (tvb_reported_length_remaining(tvb, pos) > 0)
  {
    type = tvb_get_guint8 (tvb, pos);
    tlv_tree = proto_tree_add_subtree(dcd_tree, tvb, pos, -1,
                                            ett_docsis_dcd_tlv, &tlv_item,
                                            val_to_str(type, dcd_tlv_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (tlv_tree, hf_docsis_dcd_type, tvb, pos, 1, type);
    pos++;
    proto_tree_add_item_ret_uint (tlv_tree, hf_docsis_dcd_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(tlv_item, length + 2);

    switch (type)
    {
    case DCD_DOWN_CLASSIFIER:
      dissect_dcd_down_classifier (tvb, pinfo, tlv_tree, pos, length);
      break;
    case DCD_DSG_RULE:
      dissect_dcd_dsg_rule (tvb, pinfo, tlv_tree, pos, length);
      break;
    case DCD_DSG_CONFIG:
      dissect_dcd_dsg_cfg (tvb, pinfo, tlv_tree, pos, length);
      break;
    }     /* switch(type) */

     pos += length;
  }       /* while (tvb_reported_length_remaining(tvb, pos) > 0) */

  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_docsis_dcd (void)
{
  /* Setup list of header fields  See Section 1.6.1 for details*/
  static hf_register_info hf[] = {
    {&hf_docsis_dcd_config_ch_cnt,
     {
       "Configuration Change Count",
       "docsis_dcd.config_ch_cnt",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_num_of_frag,
     {
       "Number of Fragments",
       "docsis_dcd.num_of_frag",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_frag_sequence_num,
     {
       "Fragment Sequence Number",
       "docsis_dcd.frag_sequence_num",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_type,
     {
      "Type",
      "docsis_dcd.tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_tlv_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_length,
     {
      "Length",
      "docsis_dcd.tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_down_classifier_subtype,
     {
      "Type",
      "docsis_dcd.down_classifier_tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_down_classifier_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_down_classifier_length,
     {
      "Length",
      "docsis_dcd.down_classifier_tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_cfr_id,
     {
       "Downstream Classifier Id",
       "docsis_dcd.cfr_id",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_rule_pri,
     {
       "Downstream Classifier Rule Priority",
       "docsis_dcd.cfr_rule_pri",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_subtype,
     {
      "Type",
      "docsis_dcd.cfr_ip_tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_cfr_ip_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_length,
     {
      "Length",
      "docsis_dcd.cfr_ip_tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_source_addr,
     {
       "Downstream Classifier IP Source Address",
       "docsis_dcd.cfr_ip_source_addr",
       FT_IPv4, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_source_mask,
     {
       "Downstream Classifier IP Source Mask",
       "docsis_dcd.cfr_ip_source_mask",
       FT_IPv4, BASE_NETMASK, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_dest_addr,
     {
       "Downstream Classifier IP Destination Address",
       "docsis_dcd.cfr_ip_dest_addr",
       FT_IPv4, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_ip_dest_mask,
     {
       "Downstream Classifier IP Destination Mask",
       "docsis_dcd.cfr_ip_dest_mask",
       FT_IPv4, BASE_NETMASK, NULL, 0x0,
       "Downstream Classifier IP Destination Address",
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_tcpudp_srcport_start,
     {
       "Downstream Classifier IP TCP/UDP Source Port Start",
       "docsis_dcd.cfr_ip_tcpudp_srcport_start",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_tcpudp_srcport_end,
     {
       "Downstream Classifier IP TCP/UDP Source Port End",
       "docsis_dcd.cfr_ip_tcpudp_srcport_end",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_tcpudp_dstport_start,
     {
       "Downstream Classifier IP TCP/UDP Destination Port Start",
       "docsis_dcd.cfr_ip_tcpudp_dstport_start",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfr_tcpudp_dstport_end,
     {
       "Downstream Classifier IP TCP/UDP Destination Port End",
       "docsis_dcd.cfr_ip_tcpudp_dstport_end",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_rule_id,
     {
       "DSG Rule Id",
       "docsis_dcd.rule_id",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_rule_pri,
     {
       "DSG Rule Priority",
       "docsis_dcd.rule_pri",
       FT_UINT8, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_rule_ucid_list,
     {
       "DSG Rule UCID Range",
       "docsis_dcd.rule_ucid_list",
       FT_BYTES, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_clid_subtype,
     {
      "Type",
      "docsis_dcd.clid_tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_clid_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_clid_length,
     {
      "Length",
      "docsis_dcd.clid_tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_clid_bcast_id,
     {
       "DSG Rule Client ID Broadcast ID",
       "docsis_dcd.clid_bcast_id",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_clid_known_mac_addr,
     {
       "DSG Rule Client ID Known MAC Address",
       "docsis_dcd.clid_known_mac_addr",
       FT_ETHER, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_clid_ca_sys_id,
     {
       "DSG Rule Client ID CA System ID",
       "docsis_dcd.clid_ca_sys_id",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_clid_app_id,
     {
       "DSG Rule Client ID Application ID",
       "docsis_dcd.clid_app_id",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_dsg_rule_subtype,
     {
      "Type",
      "docsis_dcd.rule_tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_dsg_rule_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_dsg_rule_length,
     {
      "Length",
      "docsis_dcd.rule_tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_rule_tunl_addr,
     {
       "DSG Rule Tunnel MAC Address",
       "docsis_dcd.rule_tunl_addr",
       FT_ETHER, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_rule_cfr_id,
     {
       "DSG Rule Classifier ID",
       "docsis_dcd.rule_cfr_id",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_rule_vendor_spec,
     {
       "DSG Rule Vendor Specific Parameters",
       "docsis_dcd.rule_vendor_spec",
       FT_BYTES, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_subtype,
     {
      "Type",
      "docsis_dcd.cfg_tlvtype",
      FT_UINT8, BASE_DEC, VALS(dcd_cfg_vals), 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_cfg_length,
     {
      "Length",
      "docsis_dcd.cfg_tlvlen",
      FT_UINT8, BASE_DEC, NULL, 0x0,
      NULL,
      HFILL
     }
    },
    {&hf_docsis_dcd_cfg_chan,
     {
       "DSG Configuration Channel",
       "docsis_dcd.cfg_chan",
       FT_UINT32, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_tdsg1,
     {
       "DSG Initialization Timeout (Tdsg1)",
       "docsis_dcd.cfg_tdsg1",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_tdsg2,
     {
       "DSG Operational Timeout (Tdsg2)",
       "docsis_dcd.cfg_tdsg2",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_tdsg3,
     {
       "DSG Two-Way Retry Timer (Tdsg3)",
       "docsis_dcd.cfg_tdsg3",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_tdsg4,
     {
       "DSG One-Way Retry Timer (Tdsg4)",
       "docsis_dcd.cfg_tdsg4",
       FT_UINT16, BASE_DEC, NULL, 0x0,
       NULL,
       HFILL
     }
    },
    {&hf_docsis_dcd_cfg_vendor_spec,
     {
       "DSG Configuration Vendor Specific Parameters",
       "docsis_dcd.cfg_vendor_spec",
       FT_BYTES, BASE_NONE, NULL, 0x0,
       NULL,
       HFILL
     }
    },

  };

  static gint *ett[] = {
    &ett_docsis_dcd,
    &ett_docsis_dcd_cfr,
    &ett_docsis_dcd_cfr_ip,
    &ett_docsis_dcd_rule,
    &ett_docsis_dcd_clid,
    &ett_docsis_dcd_cfg,
    &ett_docsis_dcd_tlv,
  };

  static ei_register_info ei[] = {
    {&ei_docsis_dcd_tlvlen_bad, { "docsis_dcd.tlvlenbad", PI_MALFORMED, PI_ERROR, "Bad TLV length", EXPFILL}},
  };

  expert_module_t* expert_docsis_dcd;

  proto_docsis_dcd =
    proto_register_protocol ("DOCSIS Downstream Channel Descriptor",
                             "DOCSIS DCD", "docsis_dcd");

  proto_register_field_array (proto_docsis_dcd, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
  expert_docsis_dcd = expert_register_protocol(proto_docsis_dcd);
  expert_register_field_array(expert_docsis_dcd, ei, array_length(ei));

  docsis_dcd_handle = register_dissector ("docsis_dcd", dissect_dcd, proto_docsis_dcd);
}

void
proto_reg_handoff_docsis_dcd (void)
{
  dissector_add_uint ("docsis_mgmt", 0x20, docsis_dcd_handle);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
