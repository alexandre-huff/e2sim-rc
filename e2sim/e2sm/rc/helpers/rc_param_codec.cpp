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

#include "rc_param_codec.hpp"
#include "logger.h"

extern "C" {
    #include "RANParameter-ValueType-Choice-Structure.h"
    #include "RANParameter-ValueType-Choice-List.h"
    #include "RANParameter-LIST.h"
}

/// @brief Creates a new RAN Parameter Structure Item
/// @param id The RAN parameter ID number
/// @return A pointer to the newly and populated RAN Parameter Structure Item, or @p nullptr on error.
RANParameter_STRUCTURE_Item_t *common::rc::build_ran_parameter_structure_item(const RANParameter_ID_t id) {
    LOGGER_TRACE_FUNCTION_IN

    RANParameter_STRUCTURE_Item_t *param = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
    if (!param) {
        logger_error("Unable to allocate memory for a new RANParameter_STRUCTURE_Item_t ID %lu", id);
        return nullptr;
    }

    param->ranParameter_ID = id;
    param->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
    param->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
    param->ranParameter_valueType->choice.ranP_Choice_Structure =
            (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));
    param->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure =
            (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
    param->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters =
            (RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
            calloc(1, sizeof(RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

    LOGGER_TRACE_FUNCTION_OUT

    return param;
}

/// @brief Creates a new RAN Parameter List. This function creates a RAN Parameter Structure Item that contains a RAN Parameter List ValueType
/// @param id The RAN parameter ID number
/// @return A pointer to the newly and populated RAN Parameter Structure Item, or @p nullptr on error.
RANParameter_STRUCTURE_Item_t *common::rc::build_ran_parameter_list(const RANParameter_ID_t id) {
    LOGGER_TRACE_FUNCTION_IN

    RANParameter_STRUCTURE_Item_t *param = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
    if (!param) {
        logger_error("Unable to allocate memory for a new RANParameter_STRUCTURE_Item_t ID %lu while building a RAN Parameter List Item", id);
        return nullptr;
    }

    param->ranParameter_ID = id;
    param->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
    param->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_List;
    param->ranParameter_valueType->choice.ranP_Choice_List =
            (RANParameter_ValueType_Choice_List_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_List_t));
    param->ranParameter_valueType->choice.ranP_Choice_List->ranParameter_List =
            (RANParameter_LIST_t *) calloc(1, sizeof(RANParameter_LIST_t));

    LOGGER_TRACE_FUNCTION_OUT

    return param;
}

/// @brief Creates a new RAN Parameter Structure that will be a container to add each of the Ran Parameter List Items
/// @return A pointer to the newly and populated RAN Parameter Structure, or @p nullptr on error.
RANParameter_STRUCTURE_t *common::rc::build_ran_parameter_list_item() {
    RANParameter_STRUCTURE_t *s = (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
    if (s) {
        s->sequence_of_ranParameters = (RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                calloc(1, sizeof(RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));
    }

    return s;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointer of @p value is not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const BOOLEAN_t *value) {
    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valueBoolean;
    v->choice.valueBoolean = *value;

    return v;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointer of @p value is not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const long *value) {
    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valueInt;
    v->choice.valueInt = *value;

    return v;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointer of @p value is not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const double *value) {
    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valueReal;
    v->choice.valueReal = *value;

    return v;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointer of @p value is not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const BIT_STRING_t *value) {
    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valueBitS;
    v->choice.valueBitS.size = value->size;
    v->choice.valueBitS.bits_unused = value->bits_unused;
    v->choice.valueBitS.buf = (uint8_t *) calloc(value->size, sizeof(uint8_t));
    memcpy(v->choice.valueBitS.buf, value->buf, value->size);

    return v;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointers of @p value and its @p data are not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const OctetStringWrapper *value) {
    if (!value || !value->data) {
        logger_error("OctetStringWrapper and its data cannot be null");
        return nullptr;
    }

    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valueOctS;
    v->choice.valueOctS.size = value->data->size;
    v->choice.valueOctS.buf = (uint8_t *) calloc(value->data->size, sizeof(uint8_t));
    memcpy(v->choice.valueOctS.buf, value->data->buf, value->data->size);

    return v;
}

/// @brief Creates a new RAN Parameter Value
/// @param value The data that is copied to the new ASN.1 RAN Parameter Value. The pointers of @p value and its @p data are not touched by this function
/// @return A pointer to the newly and populated RAN Parameter Value, or @p nullptr on error.
RANParameter_Value_t *common::rc::build_ran_parameter_value(const PrintableStringWrapper *value) {
    if (!value || !value->data) {
        logger_error("PrintableStringWrapper and its data cannot be null");
        return nullptr;
    }

    RANParameter_Value_t *v = (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
    if (!v) {
        logger_error("Unable to allocate memory for RANParameter_Value_t");
        return nullptr;
    }

    v->present = RANParameter_Value_PR_valuePrintableString;
    v->choice.valuePrintableString.size = value->data->size;
    v->choice.valuePrintableString.buf = (uint8_t *) calloc(value->data->size, sizeof(uint8_t));
    memcpy(v->choice.valuePrintableString.buf, value->data->buf, value->data->size);

    return v;
}

/// @brief Retrieves a given RAN Parameter within a sequence of RAN parameteres within a RAN Parameter Structure
/// @param ranp_s The RAN Parameter Structure which we will iterate to retrieve the RAN Parameter Structure Item
/// @param ranp_id The RAN Parameter ID we want to retrieve
/// @param ranp_type The RAN Parameter Type of the @p ranp_id
/// @return A pointer to the address of @p ranp_id structure item in @p ranp_s , or @p nullptr on error:
///         - item with a mismatching value type
///         - item not found
RANParameter_STRUCTURE_Item_t *common::rc::get_ran_parameter_structure_item(const RANParameter_STRUCTURE_t *ranp_s, RANParameter_ID_t ranp_id, RANParameter_ValueType_PR ranp_type) {
    LOGGER_TRACE_FUNCTION_IN

    if (!ranp_s) {
        logger_error("Unable to retrieve RAN Parameter ID %ld. Does RAN Parameter Structure is nil?", ranp_id);
        return nullptr;
    }

    RANParameter_STRUCTURE_Item_t *param = nullptr;

    int count = ranp_s->sequence_of_ranParameters->list.count;
    RANParameter_STRUCTURE_Item_t **params = ranp_s->sequence_of_ranParameters->list.array;
    for (int i = 0; i < count; i++) {
        if (params[i]->ranParameter_ID == ranp_id) {
            if (params[i]->ranParameter_valueType->present == ranp_type) {
                param = params[i];
            } else {
                logger_error("RAN Parameter ID %ld is not of type %d", ranp_id, ranp_type);
            }
            break;
        }
    }

    if (LOGGER_LEVEL >= LOGGER_INFO) {
        if (!param) {
            logger_info("RAN Parameter ID %ld not found in RAN Parameter Structure", ranp_id);
        }
    }

    LOGGER_TRACE_FUNCTION_OUT

    return param;
}

std::vector<RANParameter_STRUCTURE_t *> common::rc::get_ran_parameter_list_items(const RANParameter_LIST_t *ranp_list) {
	if (!ranp_list) {
        logger_error("Unable to retrieve RAN Parameter LIST items. Does RAN Parameter List is nil?");
        return std::vector<RANParameter_STRUCTURE_t *>();
    }

	std::vector<RANParameter_STRUCTURE_t *> list;

    int count = ranp_list->list_of_ranParameter.list.count;
    RANParameter_STRUCTURE_t **params = ranp_list->list_of_ranParameter.list.array;

	for (int i = 0; i < count; i++) {
		list.emplace_back(params[i]);
    }

    return list;
}
