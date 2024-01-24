/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-RC-IEs"
 * 	found in "E2SM-RC-R003-v03.00.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#include "E2SM-RC-QueryOutcome-Format1-ItemCell.h"

#include "NeighborRelation-Info.h"
#include "E2SM-RC-QueryOutcome-Format1-ItemParameters.h"
static int
memb_ranP_List_constraint_1(const asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	size_t size;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	/* Determine the number of elements */
	size = _A_CSEQUENCE_FROM_VOID(sptr)->count;
	
	if((size <= 65535)) {
		/* Perform validation of the inner elements */
		return td->encoding_constraints.general_constraints(td, sptr, ctfailcb, app_key);
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_ranP_List_constr_3 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 16,  16,  0,  65535 }	/* (SIZE(0..65535)) */,
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_ranP_List_constr_3 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 16,  16,  0,  65535 }	/* (SIZE(0..65535)) */,
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_ranP_List_3[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemParameters,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		""
		},
};
static const ber_tlv_tag_t asn_DEF_ranP_List_tags_3[] = {
	(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_ranP_List_specs_3 = {
	sizeof(struct E2SM_RC_QueryOutcome_Format1_ItemCell__ranP_List),
	offsetof(struct E2SM_RC_QueryOutcome_Format1_ItemCell__ranP_List, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ranP_List_3 = {
	"ranP-List",
	"ranP-List",
	&asn_OP_SEQUENCE_OF,
	asn_DEF_ranP_List_tags_3,
	sizeof(asn_DEF_ranP_List_tags_3)
		/sizeof(asn_DEF_ranP_List_tags_3[0]) - 1, /* 1 */
	asn_DEF_ranP_List_tags_3,	/* Same as above */
	sizeof(asn_DEF_ranP_List_tags_3)
		/sizeof(asn_DEF_ranP_List_tags_3[0]), /* 2 */
	{ 0, &asn_PER_type_ranP_List_constr_3, SEQUENCE_OF_constraint },
	asn_MBR_ranP_List_3,
	1,	/* Single element */
	&asn_SPC_ranP_List_specs_3	/* Additional specs */
};

asn_TYPE_member_t asn_MBR_E2SM_RC_QueryOutcome_Format1_ItemCell_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct E2SM_RC_QueryOutcome_Format1_ItemCell, cellGlobal_ID),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_CGI,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"cellGlobal-ID"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct E2SM_RC_QueryOutcome_Format1_ItemCell, ranP_List),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		0,
		&asn_DEF_ranP_List_3,
		0,
		{ 0, &asn_PER_memb_ranP_List_constr_3,  memb_ranP_List_constraint_1 },
		0, 0, /* No default value */
		"ranP-List"
		},
	{ ATF_POINTER, 1, offsetof(struct E2SM_RC_QueryOutcome_Format1_ItemCell, neighborRelation_Table),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NeighborRelation_Info,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"neighborRelation-Table"
		},
};
static const int asn_MAP_E2SM_RC_QueryOutcome_Format1_ItemCell_oms_1[] = { 2 };
static const ber_tlv_tag_t asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_E2SM_RC_QueryOutcome_Format1_ItemCell_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* cellGlobal-ID */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* ranP-List */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 } /* neighborRelation-Table */
};
asn_SEQUENCE_specifics_t asn_SPC_E2SM_RC_QueryOutcome_Format1_ItemCell_specs_1 = {
	sizeof(struct E2SM_RC_QueryOutcome_Format1_ItemCell),
	offsetof(struct E2SM_RC_QueryOutcome_Format1_ItemCell, _asn_ctx),
	asn_MAP_E2SM_RC_QueryOutcome_Format1_ItemCell_tag2el_1,
	3,	/* Count of tags in the map */
	asn_MAP_E2SM_RC_QueryOutcome_Format1_ItemCell_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	3,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell = {
	"E2SM-RC-QueryOutcome-Format1-ItemCell",
	"E2SM-RC-QueryOutcome-Format1-ItemCell",
	&asn_OP_SEQUENCE,
	asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1,
	sizeof(asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1)
		/sizeof(asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1[0]), /* 1 */
	asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1,	/* Same as above */
	sizeof(asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1)
		/sizeof(asn_DEF_E2SM_RC_QueryOutcome_Format1_ItemCell_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_E2SM_RC_QueryOutcome_Format1_ItemCell_1,
	3,	/* Elements count */
	&asn_SPC_E2SM_RC_QueryOutcome_Format1_ItemCell_specs_1	/* Additional specs */
};
