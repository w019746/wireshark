/* packet-cmstatus.c
 * Routines for DOCSIS 3.0 CM-STATUS Report Message dissection.
 * Copyright 2011, Hendrik Robbel <hendrik.robbel[AT]kabeldeutschland.de>
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

#define SEC_CH_MDD_TIMEOUT      1
#define QAM_FEC_LOCK_FAILURE    2
#define SEQ_OUT_OF_RANGE        3
#define SEC_CH_MDD_RECOVERY     4
#define QAM_FEC_LOCK_RECOVERY   5
#define T4_TIMEOUT              6
#define T3_RETRIES_EXCEEDED     7
#define SUCCESS_RANGING_AFTER_T3_RETRIES_EXCEEDED 8
#define CM_ON_BATTERY           9
#define CM_ON_AC_POWER         10

#define EVENT_DESCR             2
#define EVENT_DS_CH_ID          4
#define EVENT_US_CH_ID          5
#define EVENT_DSID              6

static const value_string cmstatus_tlv_vals[] = {
  {EVENT_DS_CH_ID, "Downstream Channel ID"},
  {EVENT_US_CH_ID, "Upstream Channel ID"},
  {EVENT_DSID, "DSID"},
  {EVENT_DESCR, "Description"},
  {0, NULL}
};

void proto_register_docsis_cmstatus(void);
void proto_reg_handoff_docsis_cmstatus(void);

/* Initialize the protocol and registered fields */
static int proto_docsis_cmstatus = -1;
static int hf_docsis_cmstatus_tranid = -1;
static int hf_docsis_cmstatus_e_t_mdd_t = -1;
static int hf_docsis_cmstatus_e_t_qfl_f = -1;
static int hf_docsis_cmstatus_e_t_s_o = -1;
static int hf_docsis_cmstatus_e_t_mdd_r = -1;
static int hf_docsis_cmstatus_e_t_qfl_r = -1;
static int hf_docsis_cmstatus_e_t_t4_t = -1;
static int hf_docsis_cmstatus_e_t_t3_e = -1;
static int hf_docsis_cmstatus_e_t_rng_s = -1;
static int hf_docsis_cmstatus_e_t_cm_b = -1;
static int hf_docsis_cmstatus_e_t_cm_a = -1;
static int hf_docsis_cmstatus_ds_ch_id = -1;
static int hf_docsis_cmstatus_us_ch_id = -1;
static int hf_docsis_cmstatus_dsid = -1;
static int hf_docsis_cmstatus_descr = -1;
static int hf_docsis_cmstatus_tlv_data = -1;
static int hf_docsis_cmstatus_type = -1;
static int hf_docsis_cmstatus_length = -1;

/* Initialize the subtree pointers */
static gint ett_docsis_cmstatus = -1;
static gint ett_docsis_cmstatus_tlv = -1;
static gint ett_docsis_cmstatus_tlvtlv = -1;

static expert_field ei_docsis_cmstatus_tlvlen_bad = EI_INIT;

static dissector_handle_t docsis_cmstatus_handle;

/* Dissection */
/* See Table 6-52 in CM-SP-MULPIv3.0-I14-101008 */
static void
dissect_cmstatus_tlv (tvbuff_t * tvb, packet_info* pinfo, proto_tree * tree)
{
  proto_item *it, *tlv_item, *tlv_len_item;
  proto_tree *tlv_tree;
  guint16 pos = 0;
  guint8 type;
  guint32 length;

  it = proto_tree_add_item(tree, hf_docsis_cmstatus_tlv_data, tvb, 0, tvb_reported_length(tvb), ENC_NA);
  tlv_tree = proto_item_add_subtree (it, ett_docsis_cmstatus_tlv);

  while (tvb_reported_length_remaining(tvb, pos) > 0)
  {
    type = tvb_get_guint8 (tvb, pos);
    tlv_tree = proto_tree_add_subtree(tlv_tree, tvb, pos, -1,
                                            ett_docsis_cmstatus_tlvtlv, &tlv_item,
                                            val_to_str(type, cmstatus_tlv_vals,
                                                       "Unknown TLV (%u)"));
    proto_tree_add_uint (tlv_tree, hf_docsis_cmstatus_type, tvb, pos, 1, type);
    pos++;
    tlv_len_item = proto_tree_add_item_ret_uint (tlv_tree, hf_docsis_cmstatus_length, tvb, pos, 1, ENC_NA, &length);
    pos++;
    proto_item_set_len(tlv_item, length + 2);

    switch (type)
    {
    case EVENT_DS_CH_ID:
      if (length == 3)
      {
        proto_tree_add_item (tlv_tree, hf_docsis_cmstatus_ds_ch_id, tvb, pos + 1, 1, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_cmstatus_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;

    case EVENT_US_CH_ID:
      if (length == 3)
      {
        proto_tree_add_item (tlv_tree, hf_docsis_cmstatus_us_ch_id, tvb, pos + 1, 1, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_cmstatus_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;

    case EVENT_DSID:
      if (length == 5)
      {
        proto_tree_add_item (tlv_tree, hf_docsis_cmstatus_dsid, tvb, pos + 1, 3, ENC_BIG_ENDIAN);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_cmstatus_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;

    case EVENT_DESCR:
      if (length >= 3 && length <= 82)
      {
        proto_tree_add_item (tlv_tree, hf_docsis_cmstatus_descr, tvb, pos + 1, length - 2, ENC_NA);
      }
      else
      {
        expert_add_info_format(pinfo, tlv_len_item, &ei_docsis_cmstatus_tlvlen_bad, "Wrong TLV length: %u", length);
      }
      break;
    } /* switch */
      pos += length;
  } /* while */
}

static int
dissect_cmstatus (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  proto_item *it;
  proto_tree *cmstatus_tree = NULL;
  guint32 transid;
  guint8 event_type;
  tvbuff_t* next_tvb;

  it = proto_tree_add_protocol_format (tree, proto_docsis_cmstatus, tvb, 0, -1, "CM-STATUS Report");
  cmstatus_tree = proto_item_add_subtree (it, ett_docsis_cmstatus);
  proto_tree_add_item_ret_uint (cmstatus_tree, hf_docsis_cmstatus_tranid, tvb, 0, 2, ENC_BIG_ENDIAN, &transid);

  col_add_fstr (pinfo->cinfo, COL_INFO, "CM-STATUS Report: Transaction ID = %u", transid);

  event_type = tvb_get_guint8 (tvb, 2);
  switch (event_type)
  {
  case SEC_CH_MDD_TIMEOUT:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_mdd_t, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case QAM_FEC_LOCK_FAILURE:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_qfl_f, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case SEQ_OUT_OF_RANGE:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_s_o, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case SEC_CH_MDD_RECOVERY:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_mdd_r, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case QAM_FEC_LOCK_RECOVERY:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_qfl_r, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case T4_TIMEOUT:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_t4_t, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case T3_RETRIES_EXCEEDED:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_t3_e, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

   case SUCCESS_RANGING_AFTER_T3_RETRIES_EXCEEDED:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_rng_s, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case CM_ON_BATTERY:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_cm_b, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;

  case CM_ON_AC_POWER:
    proto_tree_add_item (cmstatus_tree, hf_docsis_cmstatus_e_t_cm_a, tvb, 2, 1, ENC_BIG_ENDIAN);
    break;
  } /* switch */

  /* Call Dissector TLV's */
  next_tvb = tvb_new_subset_remaining(tvb, 3);
  dissect_cmstatus_tlv(next_tvb, pinfo, cmstatus_tree);
  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_docsis_cmstatus (void)
{
  static hf_register_info hf[] = {
    {&hf_docsis_cmstatus_tranid,
     {"Transaction ID", "docsis_cmstatus.tranid",FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL}
    },
    /* See Table 10-3 in CM-SP-MULPIv3.0-I14-101008 */

    {&hf_docsis_cmstatus_e_t_mdd_t,
     {"Secondary Channel MDD timeout", "docsis_cmstatus.mdd_timeout", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_qfl_f,
     {"QAM/FEC lock failure", "docsis_cmstatus.qam_fec_lock_failure", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_s_o,
     {"Sequence out-of-range", "docsis_cmstatus.sequence_out_of_range", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_mdd_r,
     {"Secondary Channel MDD Recovery", "docsis_cmstatus.mdd_recovery", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_qfl_r,
     {"QAM/FEC Lock Recovery", "docsis_cmstatus.qam_fec_lock_recovery", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_t4_t,
     {"T4 timeout", "docsis_cmstatus.t4_timeout", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_t3_e,
     {"T3 retries exceeded", "docsis_cmstatus.t3_retries_exceeded", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_rng_s,
     {"Successful ranging after T3 retries exceeded", "docsis_cmstatus.successful_ranging_after_t3_retries_exceeded", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_cm_b,
     {"CM operating on battery backup", "docsis_cmstatus.cm_on_battery", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_e_t_cm_a,
     {"CM returned to A/C power", "docsis_cmstatus.cm_on_ac_power", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_descr,
     {"Description", "docsis_cmstatus.description",FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_ds_ch_id,
     {"Downstream Channel ID", "docsis_cmstatus.ds_chid",FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_us_ch_id,
     {"Upstream Channel ID", "docsis_cmstatus.us_chid",FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_dsid,
     {"DSID", "docsis_cmstatus.dsid", FT_UINT24, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_tlv_data,
     {"TLV Data", "docsis_cmstatus.tlv_data", FT_BYTES, BASE_NO_DISPLAY_VALUE, NULL, 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_type,
     {"Type", "docsis_cmstatus.type",FT_UINT8, BASE_DEC, VALS(cmstatus_tlv_vals), 0x0, NULL, HFILL}
    },
    {&hf_docsis_cmstatus_length,
     {"Length", "docsis_cmstatus.length",FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
    },
  };

  static gint *ett[] = {
    &ett_docsis_cmstatus,
    &ett_docsis_cmstatus_tlv,
    &ett_docsis_cmstatus_tlvtlv,
  };

  static ei_register_info ei[] = {
    {&ei_docsis_cmstatus_tlvlen_bad, { "docsis_cmstatus.tlvlenbad", PI_MALFORMED, PI_ERROR, "Bad TLV length", EXPFILL}},
  };

  expert_module_t* expert_docsis_cmstatus;

  proto_docsis_cmstatus = proto_register_protocol ("DOCSIS CM-STATUS Report", "DOCSIS CM-STATUS", "docsis_cmstatus");

  proto_register_field_array (proto_docsis_cmstatus, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
  expert_docsis_cmstatus = expert_register_protocol(proto_docsis_cmstatus);
  expert_register_field_array(expert_docsis_cmstatus, ei, array_length(ei));

  docsis_cmstatus_handle = register_dissector ("docsis_cmstatus", dissect_cmstatus, proto_docsis_cmstatus);
}

void
proto_reg_handoff_docsis_cmstatus (void)
{
  dissector_add_uint ("docsis_mgmt", 0x29, docsis_cmstatus_handle);
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
