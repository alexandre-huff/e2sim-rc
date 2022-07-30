/*****************************************************************************
#                                                                            *
# Copyright 2020 AT&T Intellectual Property                                  *
# Copyright 2022 Alexandre Huff                                              *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#include "encode_rc.hpp"

using namespace std;

void encode_rc_function_definition(E2SM_RC_RANFunctionDefinition_t* ranfunc_def) {
    int ret;    // Temporary return value
    size_t len; // Temporary length to avoid unnecessary strlen calls

    fprintf(stderr, "in %s function\n", __func__);

    uint8_t *buf = (uint8_t*)"ORAN-E2SM-RC";	// short name
    uint8_t *buf2 = (uint8_t*)"RAN Control";	// ran function description
    uint8_t *buf3 = (uint8_t*)"1.3.6.1.4.1.53148.1.1.2.3";	// OID
    long inst = 1;								// ran function instance

    ASN_STRUCT_RESET(asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);

    len = strlen((char *) buf);
    ranfunc_def->ranFunction_Name.ranFunction_ShortName.size = len;
    ranfunc_def->ranFunction_Name.ranFunction_ShortName.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_ShortName.buf, buf, len);

    len = strlen((char *) buf2);
    ranfunc_def->ranFunction_Name.ranFunction_Description.buf = (uint8_t *)calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_Description.buf, buf2, len);
    ranfunc_def->ranFunction_Name.ranFunction_Description.size = len;

    len = strlen((char *) buf3);
    ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf, buf3, len);
    ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.size = len;

    ranfunc_def->ranFunction_Name.ranFunction_Instance = &inst;

    fprintf(stderr, "ranFunction_Name set up\n");

    ranfunc_def->ranFunctionDefinition_EventTrigger =
            (RANFunctionDefinition_EventTrigger_t *) calloc(1, sizeof(RANFunctionDefinition_EventTrigger_t));
    RANFunctionDefinition_EventTrigger_Style_Item_t *style_item =
            (RANFunctionDefinition_EventTrigger_Style_Item_t *) calloc(1, sizeof(RANFunctionDefinition_EventTrigger_Style_Item_t));
    style_item->ric_EventTriggerStyle_Type = 2; // call process breakpoint
    uint8_t *buf4 = (uint8_t *) "Call Process Breakpoint";
    len = strlen((char *) buf4);
    style_item->ric_EventTriggerStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(style_item->ric_EventTriggerStyle_Name.buf, buf4, len);
    style_item->ric_EventTriggerStyle_Name.size = len;
    style_item->ric_EventTriggerFormat_Type = 2;
    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_EventTrigger->ric_EventTriggerStyle_List.list, style_item);
    fprintf(stderr, "ranFunction_EventTrigger set up\n");

    ranfunc_def->ranFunctionDefinition_Insert =
            (RANFunctionDefinition_Insert_t *) calloc(1, sizeof(RANFunctionDefinition_Insert_t));
    RANFunctionDefinition_Insert_Item_t *insert_item =
            (RANFunctionDefinition_Insert_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Insert_Item_t));

    insert_item->ric_InsertStyle_Type = 4;
    uint8_t *insert_name = (uint8_t *) "Radio Access Control Request";
    len = strlen((char *) insert_name);
    insert_item->ric_InsertStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(insert_item->ric_InsertStyle_Name.buf, insert_name, len);
    insert_item->ric_InsertStyle_Name.size = len;

    insert_item->ric_SupportedEventTriggerStyle_Type = 2;
    insert_item->ric_ActionDefinitionFormat_Type = 3;
    insert_item->ric_IndicationHeaderFormat_Type = 2;
    insert_item->ric_IndicationMessageFormat_Type = 5;
    insert_item->ric_CallProcessIDFormat_Type = 1;

    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Insert->ric_InsertStyle_List.list, insert_item);
    fprintf(stderr, "ranFunction_Definition_Insert set up\n");

    ranfunc_def->ranFunctionDefinition_Control =
            (RANFunctionDefinition_Control_t *) calloc(1, sizeof(RANFunctionDefinition_Control_t));
    RANFunctionDefinition_Control_Item_t *ctrl_item =
            (RANFunctionDefinition_Control_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Control_Item_t));
    ctrl_item->ric_ControlStyle_Type = 4;
    uint8_t *ctrl_name = (uint8_t *) "Radio access control";
    len = strlen((char *) ctrl_name);
    ctrl_item->ric_ControlStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ctrl_item->ric_ControlStyle_Name.buf, ctrl_name, len);
    ctrl_item->ric_ControlStyle_Name.size = len;

    ctrl_item->ric_ControlHeaderFormat_Type = 1;
    ctrl_item->ric_ControlMessageFormat_Type = 1;
    ctrl_item->ric_CallProcessIDFormat_Type = (RIC_Format_Type_t *) calloc(1, sizeof(RIC_Format_Type_t));
    *ctrl_item->ric_CallProcessIDFormat_Type = 1;

    RANFunctionDefinition_Control_Action_Item_t *ctrl_act_item =
            (RANFunctionDefinition_Control_Action_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Control_Action_Item_t));
    ctrl_act_item->ric_ControlAction_ID = 1;
    uint8_t *ctrl_act_name = (uint8_t *) "UE Admission Control";
    len = strlen((char *) ctrl_act_name);
    ctrl_act_item->ric_ControlAction_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ctrl_act_item->ric_ControlAction_Name.buf, ctrl_act_name, len);
    ctrl_act_item->ric_ControlAction_Name.size = len;
    ASN_SEQUENCE_ADD(&ctrl_item->ric_ControlAction_List->list, ctrl_act_item);

    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Control->ric_ControlStyle_List.list, ctrl_item);
    fprintf(stderr, "ranFunction_Definition_Control set up\n");

    xer_fprint(stderr, &asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);

    fprintf(stderr, "end of %s\n", __func__);
}

void encode_rc_indication_message(E2SM_RC_IndicationMessage_t *ind_msg) {
    fprintf(stderr, "in %s function\n", __func__);

    char error_buf[300] = {0, };
    size_t errlen = 0;
    int ret;

    ASN_STRUCT_RESET(asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

    ind_msg->ric_indicationMessage_formats.choice.indicationMessage_Format5 =
            (E2SM_RC_IndicationMessage_Format5_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format5_t));
    ASN_STRUCT_RESET(asn_DEF_E2SM_RC_IndicationMessage_Format5, ind_msg->ric_indicationMessage_formats.choice.indicationMessage_Format5);

    ind_msg->ric_indicationMessage_formats.present =
            E2SM_RC_IndicationMessage__ric_indicationMessage_formats_PR_indicationMessage_Format5;
    E2SM_RC_IndicationMessage_Format5_Item_t *format_item =
            (E2SM_RC_IndicationMessage_Format5_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format5_Item_t));
    ASN_STRUCT_RESET(asn_DEF_E2SM_RC_IndicationMessage_Format5_Item, format_item);
    ASN_SEQUENCE_ADD(&ind_msg->ric_indicationMessage_formats.choice.indicationMessage_Format5->ranP_Requested_List.list, format_item);

    format_item->ranParameter_ID = 1; // NR CGI as in E2SM-RC v01.02 section 8.4.5.1
    format_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse =
            (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
    format_item->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;

    format_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value =
            (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    ASN_STRUCT_RESET(asn_DEF_RANParameter_Value, format_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value);

    format_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueOctS;

    NR_CGI_t *nr_cgi = (NR_CGI_t *) calloc(1, sizeof(NR_CGI_t));

    uint8_t *plmnid = (uint8_t *) "747";   // TODO it seems that we should get this from e2sim base class
    nr_cgi->pLMNIdentity.size = strlen((char *) plmnid);
    nr_cgi->pLMNIdentity.buf = (uint8_t *) calloc(nr_cgi->pLMNIdentity.size, sizeof(uint8_t));
    memcpy(nr_cgi->pLMNIdentity.buf, plmnid, nr_cgi->pLMNIdentity.size);

    nr_cgi->nRCellIdentity.buf = (uint8_t *) calloc(5, sizeof(uint8_t)); // required to have room for 36 bits
    nr_cgi->nRCellIdentity.size = 5;
    nr_cgi->nRCellIdentity.bits_unused = 4; // 40-36
    nr_cgi->nRCellIdentity.buf[0] = 0xB5; // TODO it seems that we should get this from e2sim base class
    nr_cgi->nRCellIdentity.buf[1] = 0xC6;
    nr_cgi->nRCellIdentity.buf[2] = 0x77;
    nr_cgi->nRCellIdentity.buf[3] = 0x88; // here we use 32 bits for gNB to avoid rotating bits and to keep things simple for now
    nr_cgi->nRCellIdentity.buf[4] = 0x05 << 4; // this is a dummy value (rotated to use only the 4 most significant bits)

    // memset(error_buf, 0, sizeof(error_buf));    // ensuring it is clean
    // errlen = 0;

    // ret = asn_check_constraints(&asn_DEF_NR_CGI, nr_cgi, error_buf, &errlen);
    // assert(ret == 0);
    // printf("error length %lu\n", errlen);
    // printf("error buf %s\n", error_buf);

    xer_fprint(stderr, &asn_DEF_NR_CGI, nr_cgi);

    fprintf(stderr, "NR_CGI set up\n");

    asn_codec_ctx_t *opt_cod;

    uint8_t nr_cgi_buffer[8192] = {0, };
    size_t nr_cgi_buffer_size = 8192;

    asn_enc_rval_t er =
        asn_encode_to_buffer(opt_cod,
                ATS_ALIGNED_BASIC_PER,
                &asn_DEF_NR_CGI,
                nr_cgi, nr_cgi_buffer, nr_cgi_buffer_size);

    fprintf(stderr, "er encded is %ld\n", er.encoded);
    fprintf(stderr, "after encoding NR_CGI\n");

    OCTET_STRING_t *ostr = &format_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueOctS;
    ostr->size = er.encoded;
    ostr->buf = (uint8_t *) calloc(er.encoded, sizeof(uint8_t));
    memcpy(ostr->buf, nr_cgi_buffer, er.encoded);

    fprintf(stderr, "here is the NR_CGI ostr: %s\n", ostr->buf);
    fprintf(stderr, "after printing NR_CGI ostr\n");

    asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage_Format5_Item, format_item);

    // memset(error_buf, 0, sizeof(error_buf));    // ensuring it is clean
    // errlen = 0;
    // ret = asn_check_constraints(&asn_DEF_E2SM_RC_IndicationMessage_Format5_Item, format_item, error_buf, &errlen);
    // assert(ret == 0);

    // asn_check_constraints(&asn_DEF_E2SM_RC_IndicationMessage, ind_msg, error_buf, &errlen);
    // assert(ret == 0);
    // printf("error length %lu\n", errlen);
    // printf("error buf %s\n", error_buf);

    asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

    fprintf(stderr, "E2SM_RC_IndicationMessage set up\n");

    xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

    fprintf(stderr, "end of %s\n", __func__);
}

void encode_rc_indication_header(E2SM_RC_IndicationHeader_t *ind_header) {
    fprintf(stderr, "in %s function\n", __func__);

    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2 =
            (E2SM_RC_IndicationHeader_Format2_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_Format2_t));
    ind_header->ric_indicationHeader_formats.present =
            E2SM_RC_IndicationHeader__ric_indicationHeader_formats_PR_indicationHeader_Format2;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ric_InsertStyle_Type = 4;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ric_InsertIndication_ID = 1;

    UEID_GNB_t *ueid_gnb = (UEID_GNB_t *) calloc(1, sizeof(UEID_GNB_t));
    ASN_STRUCT_RESET(asn_DEF_UEID_GNB, ueid_gnb);
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ueID.choice.gNB_UEID = ueid_gnb;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ueID.present = UEID_PR_gNB_UEID;
    // an integer between 0..2^40-1, but we only alloc 1 byte to store values between 0..255
    ueid_gnb->amf_UE_NGAP_ID.buf = (uint8_t *) calloc(1, sizeof(uint8_t));
    ueid_gnb->amf_UE_NGAP_ID.buf[0] = (uint8_t) 1;
    ueid_gnb->amf_UE_NGAP_ID.size = sizeof(uint8_t);

    uint8_t *plmnid = (uint8_t *) "747";   // TODO it seems that we should get this from e2sim base class
    ueid_gnb->guami.pLMNIdentity.size = strlen((char *) plmnid);
    ueid_gnb->guami.pLMNIdentity.buf = (uint8_t *) calloc(ueid_gnb->guami.pLMNIdentity.size, sizeof(uint8_t));
    memcpy(ueid_gnb->guami.pLMNIdentity.buf, plmnid, ueid_gnb->guami.pLMNIdentity.size);

    ueid_gnb->guami.aMFRegionID.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (8 bits)
    ueid_gnb->guami.aMFRegionID.buf[0] = (uint8_t) 128; // this is a dummy value
    ueid_gnb->guami.aMFRegionID.size = 1;
    ueid_gnb->guami.aMFRegionID.bits_unused = 0;

    ueid_gnb->guami.aMFSetID.buf = (uint8_t *) calloc(2, sizeof(uint8_t)); // (10 bits)
    uint16_t v = (uint16_t) 4; // this is a dummy vale (uint16_t is required to have room for 10 bits)
    v = v << 6; // we are only interested in 10 bits, so rotate them to the correct place
    ueid_gnb->guami.aMFSetID.buf[0] = (v >> 8); // only interested in the most significant bits (& 0x00ff only required for signed)
    ueid_gnb->guami.aMFSetID.buf[1] = v & 0x00ff; // we are only interested in the least significant bits
    ueid_gnb->guami.aMFSetID.size = 2;
    ueid_gnb->guami.aMFSetID.bits_unused = 6;

    ueid_gnb->guami.aMFPointer.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (6 bits)
    ueid_gnb->guami.aMFPointer.buf[0] = (uint8_t) 1 << 2; // this is a dummy value
    ueid_gnb->guami.aMFPointer.size = 1;
    ueid_gnb->guami.aMFPointer.bits_unused = 2;

    char error_buf[300] = {0, };
    size_t errlen = 0;

    asn_check_constraints(&asn_DEF_E2SM_RC_IndicationHeader, ind_header, error_buf, &errlen);
    printf("error length %lu\n", errlen);
    printf("error buf %s\n", error_buf);

    fprintf(stderr, "E2SM_RC_IndicationHeader set up\n");

    xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationHeader, ind_header);

    fprintf(stderr, "end of %s\n", __func__);
}
