/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-RC-IEs"
 * 	found in "e2sm-rc-v01.02.03.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#include "RANFunctionDefinition-Insert-Item.h"

#include "RANFunctionDefinition-Insert-Indication-Item.h"
static int
memb_ric_InsertIndication_List_constraint_1(const asn_TYPE_descriptor_t *td, const void *sptr,
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
	
	if((size >= 1 && size <= 65535)) {
		/* Perform validation of the inner elements */
		return td->encoding_constraints.general_constraints(td, sptr, ctfailcb, app_key);
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_ric_InsertIndication_List_constr_6 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 16,  16,  1,  65535 }	/* (SIZE(1..65535)) */,
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_ric_InsertIndication_List_constr_6 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 16,  16,  1,  65535 }	/* (SIZE(1..65535)) */,
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_ric_InsertIndication_List_6[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_RANFunctionDefinition_Insert_Indication_Item,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		""
		},
};
static const ber_tlv_tag_t asn_DEF_ric_InsertIndication_List_tags_6[] = {
	(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_ric_InsertIndication_List_specs_6 = {
	sizeof(struct RANFunctionDefinition_Insert_Item__ric_InsertIndication_List),
	offsetof(struct RANFunctionDefinition_Insert_Item__ric_InsertIndication_List, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_ric_InsertIndication_List_6 = {
	"ric-InsertIndication-List",
	"ric-InsertIndication-List",
	&asn_OP_SEQUENCE_OF,
	asn_DEF_ric_InsertIndication_List_tags_6,
	sizeof(asn_DEF_ric_InsertIndication_List_tags_6)
		/sizeof(asn_DEF_ric_InsertIndication_List_tags_6[0]) - 1, /* 1 */
	asn_DEF_ric_InsertIndication_List_tags_6,	/* Same as above */
	sizeof(asn_DEF_ric_InsertIndication_List_tags_6)
		/sizeof(asn_DEF_ric_InsertIndication_List_tags_6[0]), /* 2 */
	{ 0, &asn_PER_type_ric_InsertIndication_List_constr_6, SEQUENCE_OF_constraint },
	asn_MBR_ric_InsertIndication_List_6,
	1,	/* Single element */
	&asn_SPC_ric_InsertIndication_List_specs_6	/* Additional specs */
};

asn_TYPE_member_t asn_MBR_RANFunctionDefinition_Insert_Item_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_InsertStyle_Type),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Style_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-InsertStyle-Type"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_InsertStyle_Name),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Style_Name,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-InsertStyle-Name"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_SupportedEventTriggerStyle_Type),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Style_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-SupportedEventTriggerStyle-Type"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_ActionDefinitionFormat_Type),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Format_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-ActionDefinitionFormat-Type"
		},
	{ ATF_POINTER, 1, offsetof(struct RANFunctionDefinition_Insert_Item, ric_InsertIndication_List),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		0,
		&asn_DEF_ric_InsertIndication_List_6,
		0,
		{ 0, &asn_PER_memb_ric_InsertIndication_List_constr_6,  memb_ric_InsertIndication_List_constraint_1 },
		0, 0, /* No default value */
		"ric-InsertIndication-List"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_IndicationHeaderFormat_Type),
		(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Format_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-IndicationHeaderFormat-Type"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_IndicationMessageFormat_Type),
		(ASN_TAG_CLASS_CONTEXT | (6 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Format_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-IndicationMessageFormat-Type"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RANFunctionDefinition_Insert_Item, ric_CallProcessIDFormat_Type),
		(ASN_TAG_CLASS_CONTEXT | (7 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RIC_Format_Type,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ric-CallProcessIDFormat-Type"
		},
};
static const int asn_MAP_RANFunctionDefinition_Insert_Item_oms_1[] = { 4 };
static const ber_tlv_tag_t asn_DEF_RANFunctionDefinition_Insert_Item_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_RANFunctionDefinition_Insert_Item_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* ric-InsertStyle-Type */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* ric-InsertStyle-Name */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* ric-SupportedEventTriggerStyle-Type */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* ric-ActionDefinitionFormat-Type */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 }, /* ric-InsertIndication-List */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 5, 0, 0 }, /* ric-IndicationHeaderFormat-Type */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 6, 0, 0 }, /* ric-IndicationMessageFormat-Type */
    { (ASN_TAG_CLASS_CONTEXT | (7 << 2)), 7, 0, 0 } /* ric-CallProcessIDFormat-Type */
};
asn_SEQUENCE_specifics_t asn_SPC_RANFunctionDefinition_Insert_Item_specs_1 = {
	sizeof(struct RANFunctionDefinition_Insert_Item),
	offsetof(struct RANFunctionDefinition_Insert_Item, _asn_ctx),
	asn_MAP_RANFunctionDefinition_Insert_Item_tag2el_1,
	8,	/* Count of tags in the map */
	asn_MAP_RANFunctionDefinition_Insert_Item_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	8,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_RANFunctionDefinition_Insert_Item = {
	"RANFunctionDefinition-Insert-Item",
	"RANFunctionDefinition-Insert-Item",
	&asn_OP_SEQUENCE,
	asn_DEF_RANFunctionDefinition_Insert_Item_tags_1,
	sizeof(asn_DEF_RANFunctionDefinition_Insert_Item_tags_1)
		/sizeof(asn_DEF_RANFunctionDefinition_Insert_Item_tags_1[0]), /* 1 */
	asn_DEF_RANFunctionDefinition_Insert_Item_tags_1,	/* Same as above */
	sizeof(asn_DEF_RANFunctionDefinition_Insert_Item_tags_1)
		/sizeof(asn_DEF_RANFunctionDefinition_Insert_Item_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_RANFunctionDefinition_Insert_Item_1,
	8,	/* Elements count */
	&asn_SPC_RANFunctionDefinition_Insert_Item_specs_1	/* Additional specs */
};

