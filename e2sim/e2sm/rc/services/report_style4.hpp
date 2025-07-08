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

#ifndef RRC_STATE_OBSERVER_HPP
#define RRC_STATE_OBSERVER_HPP

#include <string>
#include <any>
#include <vector>
#include <memory>

#include "ric_indication.hpp"
#include "subaction.hpp"
#include "types_rc.hpp"
#include "ric_subscription.hpp"
#include "encode_rc.hpp"
#include "global_data.hpp"
#include "subscription_param_tree.hpp"
#include "observer.hpp"
#include "ofh_data.hpp"
#include "ofh_du.hpp"

extern "C" {
    #include "UEID.h"
    #include "E2SM-RC-IndicationMessage-Format2-RANParameter-Item.h"
    #include "RANParameter-ID.h"
}

/**
 * This class implements the E2SM-RC REPORT Service Style 4 feature
*/
class ReportStyle4 : public Observer<e2sim::ofh::MessageTypes>,
                     public SubscriptionAction {
public:
    ReportStyle4(ric_subscription_info_t info, OfhDuServer &ofh_du, std::any style4_data,
            std::shared_ptr<RICIndicationProcedure> ric_indication, const std::shared_ptr<GlobalE2NodeData> &global_data) :
            subscriptionInfo(info), ofhDu(ofh_du), style4Data(std::any_cast<common::rc::report_style4_data>(style4_data)),
            ricIndication(ric_indication), globalE2NodeData(global_data) { /* pass */ }
    ~ReportStyle4() { /* pass */ }

    bool update(e2sim::ofh::MessageTypes event, const std::any &subject) override;

    /**
     * Starts the RRCStateObserver subscription action
     *
     * FIXME check this documentation
     * IMPORTANT: first set the shared_ptr that owns this object
    */
    virtual bool start() override;

    /**
     * Stops the RRCStateObserver subscription action
    */
    virtual bool stop() override;

    bool generate_ueid_report_info(UEID_t &ueid, const std::string &imsi);

    bool generate_ran_params_report_info(const e2sim::ofh::cell_metrics_t &primary_cell,
                                        const std::vector<e2sim::ofh::cell_metrics_t> &neighbor_cells,
                                        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> &params);

    RANParameter_STRUCTURE_Item_t *generate_NRCell_report_info(const std::shared_ptr<TreeNode> param2add,
            const std::string &mcc, const std::string &mnc, const uint32_t gnb_id, uint16_t pci, const long rsrp, const long rsrq, const long sinr);

    void encode_and_send_report_msg(std::vector<common::rc::indication_msg_format2_ueid_t> &ue_ids);

private:
    bool isStarted = false;
    uint16_t counter = 0; // Indication sequence number counter
    ric_subscription_info_t subscriptionInfo;
    common::rc::report_style4_data style4Data;
    std::shared_ptr<RICIndicationProcedure> ricIndication;
    OfhDuServer &ofhDu;
    std::shared_ptr<GlobalE2NodeData> globalE2NodeData;

    bool update_registration_request(e2sim::ofh::ue_registration_request_t &subject);
    bool update_deregistration_request(e2sim::ofh::ue_deregistration_request_t &subject);
    bool update_metrics_request(e2sim::ofh::ue_metrics_request_t &subject);
};

#endif
