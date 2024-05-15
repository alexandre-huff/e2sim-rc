/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
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

#ifndef E2SM_RC_PARAM_CODEC_HPP
#define E2SM_RC_PARAM_CODEC_HPP

#include <type_traits>
#include <typeinfo>

#include "logger.h"

extern "C" {
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-ID.h"
    #include "RANParameter-Value.h"
    #include "RANParameter-ValueType.h"
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-ValueType-Choice-ElementTrue.h"
}

namespace common{
    namespace rc {

        /**
         * Wrapper for OCTET_STRING_t data required to avoid conflicts on function overload
         * due to typedef redefinitions that points to OCTET_STRING_t type.
        */
        struct OctetStringWrapper {
            OCTET_STRING_t *data;
        };
        /**
         * Wrapper for PrintableString_t data required to avoid conflicts on function overload
         * due to typedef redefinitions that points to OCTET_STRING type.
         * Printable_String_t is an alias of OCTET_STRING_t.
        */
        struct PrintableStringWrapper {
            PrintableString_t *data;
        };

        /**
         * Implements the concept of all possible types accepted for RANParameter_Value_u union, as per defined in RANParameter-Value.h
        */
        template<typename T>
        concept RANParameterValueType = std::is_same_v<T, BOOLEAN_t>
                                    || std::is_same_v<T, long>
                                    || std::is_same_v<T, double>
                                    || std::is_same_v<T, BIT_STRING_t>
                                    || std::is_same_v<T, OctetStringWrapper>   // Wrapper prevents to pass an OCTET_STRING_t alias (e.g. PrintableString_t) unintentionally
                                    || std::is_same_v<T, PrintableStringWrapper>; // Wrapper required to differentiate PrintableString_t from OCTET_STRING_t

        typedef enum {
            RAN_PARAMETER_STRUCT = RANParameter_ValueType_PR_ranP_Choice_Structure,
            RAN_PARAMETER_LIST = RANParameter_ValueType_PR_ranP_Choice_List
        } RANParameterContainer_e;

        typedef enum {
            RAN_PARAMETER_ELEM_TRUE = RANParameter_ValueType_PR_ranP_Choice_ElementTrue,
            RAN_PARAMETER_ELEM_FALSE = RANParameter_ValueType_PR_ranP_Choice_ElementFalse
        } RANParameterElement_e;

        RANParameter_STRUCTURE_Item_t *build_ran_parameter_structure_item(const RANParameter_ID_t id, RANParameterContainer_e type);


        RANParameter_Value_t *build_ran_parameter_value(const BOOLEAN_t *value);
        RANParameter_Value_t *build_ran_parameter_value(const long *value);
        RANParameter_Value_t *build_ran_parameter_value(const double *value);
        RANParameter_Value_t *build_ran_parameter_value(const BIT_STRING_t *value);
        RANParameter_Value_t *build_ran_parameter_value(const OctetStringWrapper *value);
        RANParameter_Value_t *build_ran_parameter_value(const PrintableStringWrapper *value);

        /// @brief Creates a new RAN Parameter Structure Item
        /// @tparam T The concept of all possible types accepted
        /// @param id The RAN parameter ID number
        /// @param type The type of the RAN Parameter Structure we are building
        /// @param value The data that is copied to the new ASN.1 RAN Parameter Item. The pointer of @p value is not touched by this function
        /// @return A pointer to the newly and populated RAN Parameter Structure Item, or @p nullptr on error.
        template<common::rc::RANParameterValueType T>
        RANParameter_STRUCTURE_Item_t *build_ran_parameter_structure_elem_item(const RANParameter_ID_t id, RANParameterElement_e type, const T *value) {
            /*
                Please check why this implementation is required to be in the header file
                Source:
                https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl
                https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file/495056#495056
            */
            LOGGER_TRACE_FUNCTION_IN

            RANParameter_STRUCTURE_Item_t *param = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
            if (!param) {
                logger_error("Unable to allocate memory for a new RANParameter_STRUCTURE_Item_t ID %lu", id);
                return nullptr;
            }

            param->ranParameter_ID = id;
            param->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));

            if (type == common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE) {
                param->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                param->ranParameter_valueType->choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));

                param->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value = build_ran_parameter_value(value);

                if (!param->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value) {
                    logger_error("Cannot build RAN Parameter Value for RAN Parameter ID %lu", id);
                    ASN_STRUCT_FREE(asn_DEF_RANParameter_STRUCTURE_Item, param);
                    param = nullptr;
                }

            } else {    // for now, can only be common::rc::RANParameterContainer_e::RAN_PARAMETER_ELEM_TRUE
                param->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementTrue;
                param->ranParameter_valueType->choice.ranP_Choice_ElementTrue =
                        (RANParameter_ValueType_Choice_ElementTrue_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementTrue_t));

                RANParameter_Value_t *pvalue = build_ran_parameter_value(value);
                if (pvalue) {
                    param->ranParameter_valueType->choice.ranP_Choice_ElementTrue->ranParameter_value = *pvalue;
                    free(pvalue);
                } else {
                    logger_error("Cannot build RAN Parameter Value for RAN Parameter ID %lu", id);
                    ASN_STRUCT_FREE(asn_DEF_RANParameter_STRUCTURE_Item, param);
                    param = nullptr;
                }

            }

            LOGGER_TRACE_FUNCTION_OUT

            return param;
        }

        RANParameter_STRUCTURE_Item_t *get_ran_parameter_structure_item(const RANParameter_STRUCTURE_t *ranp_s, RANParameter_ID_t ranp_id, RANParameter_ValueType_PR ranp_type);

        // FIXME still need to fix return types of RANParameterValueType that require wrappers (function overloading?)
        // /// @brief Retrieves the data of a given RAN Parameter Value
        // /// @tparam T Return type of the data
        // /// @param v The ASN1 RAN Parameter Value
        // /// @param vtype The type that the ASN1 RAN Parameter Value must match
        // /// @return A pointer to the address of @p T in @p v , or @p nullptr on error:
        // ///         - RAN Parameter Value type mismatching @p vtype
        // ///         - Invalid @p vtype enum value
        // template<common::rc::RANParameterValueType T>
        // T *get_ran_parameter_value_data(const RANParameter_Value_t *v, RANParameter_Value_PR vtype) {
        //     /*
        //         Please check why this implementation is required to be in the header file
        //         Source:
        //         https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl
        //         https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file/495056#495056
        //     */
        //     LOGGER_TRACE_FUNCTION_IN

        //     T *value;

        //     if (!v) {
        //         logger_error("RAN Parameter Value is required. nil?");
        //         return nullptr;
        //     }

        //     if (v->present != vtype) {
        //         logger_error("RAN Parameter Value Type does not match with type %d", vtype);
        //         return nullptr;
        //     }

        //     switch (v->present) {
        //         case RANParameter_Value_PR_valueBoolean:
        //             value = (T *)&v->choice.valueBoolean;
        //             break;
        //         case RANParameter_Value_PR_valueInt:
        //             value = (T *)&v->choice.valueInt;
        //             break;
        //         case RANParameter_Value_PR_valueReal:
        //             value = (T *)&v->choice.valueReal;
        //             break;
        //         case RANParameter_Value_PR_valueBitS:
        //             value = (T *)&v->choice.valueBitS;
        //             break;
        //         case RANParameter_Value_PR_valueOctS:
        //             value = (T *)&v->choice.valueOctS;
        //             break;
        //         case RANParameter_Value_PR_valuePrintableString:
        //             value = (T *)&v->choice.valuePrintableString;
        //             break;
        //         case RANParameter_Value_PR_NOTHING:
        //         default:
        //             logger_error("Invalid RANParameter value PR %d in %s", v->present, asn_DEF_RANParameter_Value.name);
        //             value = nullptr;
        //     }

        //     LOGGER_TRACE_FUNCTION_OUT

        //     return value;
        // }

    }
}

#endif
