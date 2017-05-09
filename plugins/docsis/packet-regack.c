/* packet-regack.c
 * Routines for Registration Acknowledge Message dissection
 * Copyright 2002, Anand V. Narwani <anand[AT]narwani.org>
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

void proto_register_docsis_regack(void);
void proto_reg_handoff_docsis_regack(void);

/* Initialize the protocol and registered fields */
static int proto_docsis_regack = -1;
static int hf_docsis_regack_sid = -1;
static int hf_docsis_regack_response = -1;
static dissector_handle_t docsis_tlv_handle;

/* Defined in packet-tlv.c */
extern value_string docsis_conf_code[];

/* Initialize the subtree pointers */
static gint ett_docsis_regack = -1;

static dissector_handle_t docsis_regack_handle;

/* Dissection */
static int
dissect_regack (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  proto_item *it;
  proto_tree *regack_tree;
  guint32 sid, response;
  tvbuff_t *next_tvb;

  it = proto_tree_add_item (tree, proto_docsis_regack, tvb, 0, -1, ENC_NA);
  regack_tree = proto_item_add_subtree (it, ett_docsis_regack);

  proto_tree_add_item_ret_uint (regack_tree, hf_docsis_regack_sid, tvb, 0, 2, ENC_BIG_ENDIAN, &sid);
  proto_tree_add_item_ret_uint (regack_tree, hf_docsis_regack_response, tvb, 2, 1, ENC_BIG_ENDIAN, &response);

  col_add_fstr (pinfo->cinfo, COL_INFO,
                "Registration Acknowledge SID = %u (%s)", sid,
                val_to_str (response, docsis_conf_code, "%d"));

  /* Call Dissector for Appendix C TLV's */
  next_tvb = tvb_new_subset_remaining (tvb, 3);
  call_dissector (docsis_tlv_handle, next_tvb, pinfo, regack_tree);
  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_docsis_regack (void)
{
  static hf_register_info hf[] = {
    {&hf_docsis_regack_sid,
     {"Service Identifier", "docsis_regack.sid",
      FT_UINT16, BASE_DEC, NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_docsis_regack_response,
     {"Response Code", "docsis_regack.respnse",
      FT_UINT8, BASE_DEC, VALS (docsis_conf_code), 0x0,
      NULL, HFILL}
    },
  };

  static gint *ett[] = {
    &ett_docsis_regack,
  };

  proto_docsis_regack =
    proto_register_protocol ("DOCSIS Registration Acknowledge",
                             "DOCSIS REG-ACK", "docsis_regack");

  proto_register_field_array (proto_docsis_regack, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  docsis_regack_handle = register_dissector ("docsis_regack", dissect_regack, proto_docsis_regack);
}

void
proto_reg_handoff_docsis_regack (void)
{
  docsis_tlv_handle = find_dissector ("docsis_tlv");
  dissector_add_uint ("docsis_mgmt", 0x0e, docsis_regack_handle);
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
