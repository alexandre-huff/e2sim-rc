/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2AP-PDU-Contents"
 * 	found in "e2ap-v02.02.03.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#include "E2connectionUpdate.h"

asn_TYPE_member_t asn_MBR_E2connectionUpdate_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct E2connectionUpdate, protocolIEs),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ProtocolIE_Container_2007P15,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"protocolIEs"
		},
};
static const ber_tlv_tag_t asn_DEF_E2connectionUpdate_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_E2connectionUpdate_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 } /* protocolIEs */
};
asn_SEQUENCE_specifics_t asn_SPC_E2connectionUpdate_specs_1 = {
	sizeof(struct E2connectionUpdate),
	offsetof(struct E2connectionUpdate, _asn_ctx),
	asn_MAP_E2connectionUpdate_tag2el_1,
	1,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	1,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_E2connectionUpdate = {
	"E2connectionUpdate",
	"E2connectionUpdate",
	&asn_OP_SEQUENCE,
	asn_DEF_E2connectionUpdate_tags_1,
	sizeof(asn_DEF_E2connectionUpdate_tags_1)
		/sizeof(asn_DEF_E2connectionUpdate_tags_1[0]), /* 1 */
	asn_DEF_E2connectionUpdate_tags_1,	/* Same as above */
	sizeof(asn_DEF_E2connectionUpdate_tags_1)
		/sizeof(asn_DEF_E2connectionUpdate_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_E2connectionUpdate_1,
	1,	/* Elements count */
	&asn_SPC_E2connectionUpdate_specs_1	/* Additional specs */
};

