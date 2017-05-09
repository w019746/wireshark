/* packet-regrsp.c
 * Routines for Registration Response Message dissection
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

void proto_register_docsis_regrsp(void);
void proto_reg_handoff_docsis_regrsp(void);

/* Initialize the protocol and registered fields */
static int proto_docsis_regrsp = -1;
static int hf_docsis_regrsp_sid = -1;
static int hf_docsis_regrsp_response = -1;
static dissector_handle_t docsis_tlv_handle;

/* Defined in packet-tlv.c */
extern value_string docsis_conf_code[];

/* Initialize the subtree pointers */
static gint ett_docsis_regrsp = -1;

static dissector_handle_t docsis_regrsp_handle;

/* Dissection */
static int
dissect_regrsp (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  proto_item *it;
  proto_tree *regrsp_tree;
  guint32 sid, response;
  tvbuff_t *next_tvb;

  it = proto_tree_add_item(tree, proto_docsis_regrsp, tvb, 0, -1, ENC_NA);
  regrsp_tree = proto_item_add_subtree (it, ett_docsis_regrsp);
  proto_tree_add_item_ret_uint (regrsp_tree, hf_docsis_regrsp_sid, tvb, 0, 2, ENC_BIG_ENDIAN, &sid);
  proto_tree_add_item_ret_uint (regrsp_tree, hf_docsis_regrsp_response, tvb, 2, 1, ENC_BIG_ENDIAN, &response);

  col_add_fstr (pinfo->cinfo, COL_INFO,
                "Registration Response SID = %u (%s)", sid,
                val_to_str (response, docsis_conf_code, "%d"));

  /* Call Dissector for Appendix C TLV's */
  next_tvb = tvb_new_subset_remaining (tvb, 3);
  call_dissector (docsis_tlv_handle, next_tvb, pinfo, regrsp_tree);
  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_docsis_regrsp (void)
{
  static hf_register_info hf[] = {
    {&hf_docsis_regrsp_sid,
     {"Service Identifier", "docsis_regrsp.sid",
      FT_UINT16, BASE_DEC, NULL, 0x0,
      NULL, HFILL}
    },
    {&hf_docsis_regrsp_response,
     {"Response Code", "docsis_regrsp.respnse",
      FT_UINT8, BASE_DEC, VALS (docsis_conf_code), 0x0,
      NULL, HFILL}
    },
  };

  static gint *ett[] = {
    &ett_docsis_regrsp,
  };

  proto_docsis_regrsp =
    proto_register_protocol ("DOCSIS Registration Responses", "DOCSIS REG-RSP",
                             "docsis_regrsp");

  proto_register_field_array (proto_docsis_regrsp, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  docsis_regrsp_handle = register_dissector ("docsis_regrsp", dissect_regrsp, proto_docsis_regrsp);
}

void
proto_reg_handoff_docsis_regrsp (void)
{
  docsis_tlv_handle = find_dissector ("docsis_tlv");
  dissector_add_uint ("docsis_mgmt", 0x07, docsis_regrsp_handle);
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
