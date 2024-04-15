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

#ifndef GLOBAL_DATA_HPP
#define GLOBAL_DATA_HPP

#include <string>
#include <mutex>

extern "C" {
    #include "GlobalE2node-ID.h"
    #include "PLMN-Identity.h"
    #include "OCTET_STRING.h"
    #include "BIT_STRING.h"
}

/**
 * This class indicates that O-RU supports TX gain reference level control
 *
 * As per O-RAN.WG4.MP-YANGs-R003-v14.00 -> RU Specific Models/Operations/o-ran-uplane-conf.yang
*/
class TxReferenceLevel {
public:
    TxReferenceLevel(double tx_gain=0.0) : gain(tx_gain) {};

    void setGain(double tx_gain) {
        std::lock_guard<std::mutex> guard(gain_lock);
        gain = tx_gain;
    }
    double getGain() {
        std::lock_guard<std::mutex> guard(gain_lock);
        return gain;
    }

private:
    /**
     * From o-ran-uplane-conf.yang
     * The following is under module o-ran-uplane-conf->uplane-conf-group->tx-array-carriers
     * leaf gain {
     *   type decimal64 {
     *     fraction-digits 4;
     *    }
     *   units dB;
     *   mandatory true;
     *   description
     *     "Transmission gain in dB. Value applicable to each array element carrier belonging to array carrier.
     *
     *      The value of transmission gain shall meet the constraints defined in CUS-Plane, clause 8.1.3.3.";
     * }
     *
     * RFC 7950 defines grouping as:
     * "grouping: A reusable set of schema nodes, which may be used
     * locally in the module and by other modules that import from it.
     * The "grouping" statement is not a data definition statement and,
     * as such, does not define any nodes in the schema tree."
     *
     * Since uplane-conf-group does not map to a node in restconf notation,
     * then "gain" can be mapped to RESTCONF as follows:
     * http://ip:port/restconf/data/o-ran-uplane-conf HTTP/1.1
     * {
     *      o-ran-uplane-conf: {
     *          tx-array-carriers: [
     *              {gain: double_value} // max is 24dB
     *          ]
     *      }
     * }
    */
    double gain;    // max output power is 24 dB as per TS 38141-1 section 6.2
    // double max;    // Maximum of supported gain reference level in dB
    // double min;    // Minimum of supported gain reference level in dB

    std::mutex gain_lock;   // prevents the multithreaded O1 interface to run into race conditions
};

class GlobalE2NodeData {
public:
    GlobalE2NodeData(std::string mcc, std::string mnc, uint32_t gnb_id);
    ~GlobalE2NodeData();

    GlobalE2node_ID_t *getGlobalE2NodeId();
    PLMN_Identity_t *getGlobalE2NodePlmnId();
    BIT_STRING_t *getGlobalE2Node_gNBId();

    TxReferenceLevel txLevel;

    const uint32_t gnbid;

private:
    GlobalE2node_ID_t *globalE2NodeId;

};



#endif
