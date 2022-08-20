/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-COMMON-IEs"
 * 	found in "e2sm-v02.01.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#include "AMF-UE-NGAP-ID.h"

int
AMF_UE_NGAP_ID_constraint(const asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	const INTEGER_t *st = (const INTEGER_t *)sptr;
	long value;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	if(asn_INTEGER2long(st, &value)) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value too large (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	if((value >= 0 && value <= 1099511627775)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

/*
 * This type is implemented using INTEGER,
 * so here we adjust the DEF accordingly.
 */
asn_per_constraints_t asn_PER_type_AMF_UE_NGAP_ID_constr_1 CC_NOTUSED = {
	{ APC_CONSTRAINED,	 40, -1,  0,  1099511627775 }	/* (0..1099511627775) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static const ber_tlv_tag_t asn_DEF_AMF_UE_NGAP_ID_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (2 << 2))
};
asn_TYPE_descriptor_t asn_DEF_AMF_UE_NGAP_ID = {
	"AMF-UE-NGAP-ID",
	"AMF-UE-NGAP-ID",
	&asn_OP_INTEGER,
	asn_DEF_AMF_UE_NGAP_ID_tags_1,
	sizeof(asn_DEF_AMF_UE_NGAP_ID_tags_1)
		/sizeof(asn_DEF_AMF_UE_NGAP_ID_tags_1[0]), /* 1 */
	asn_DEF_AMF_UE_NGAP_ID_tags_1,	/* Same as above */
	sizeof(asn_DEF_AMF_UE_NGAP_ID_tags_1)
		/sizeof(asn_DEF_AMF_UE_NGAP_ID_tags_1[0]), /* 1 */
	{ 0, &asn_PER_type_AMF_UE_NGAP_ID_constr_1, AMF_UE_NGAP_ID_constraint },
	0, 0,	/* No members */
	0	/* No specifics */
};

