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
#include <unordered_map>
#include <vector>
#include <memory>

#define GNB_ID_LENGTH 29    // for now we use 29 bits to identify an E2 Node

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
class Cell {
public:
    Cell(uint16_t pci, double tx_gain=46.0, int socket=-1) : pci(pci), gain(tx_gain), socket(socket) {};

    /* Set the Cell's TX Reference Level in db */
    void setGain(double tx_gain) {
        gain = tx_gain;
    }

    /* Returns the Cell's TX Reference Level in db */
    double getGain() {
        return gain;
    }

    uint16_t getPci() {
        return pci;
    }

    void setSocket(int socket) {
        this->socket = socket;
    }

    int getSocket() {
        return socket;
    }

private:
    /**
     * From o-ran-uplane-conf.yang
     * The following is under module o-ran-uplane-conf->uplane-conf-group->tx-array-carriers
     * leaf gain {
     *   type decimal64 {
     *     fraction-digits 4;
     *   }
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

    uint16_t pci;   // Physical Cell Identifier
    int socket;     // Connection to the corresponding O-RU simulator
};

namespace e2sim {
namespace ue {
    struct UEInfo {
        std::string imsi;
        std::shared_ptr<Cell> connectedCell;
    };
}
}

class UEList {
public:
    void addUE(std::shared_ptr<e2sim::ue::UEInfo> ue);
    void removeUE(std::string imsi);
    std::shared_ptr<e2sim::ue::UEInfo> getUEInfo(std::string imsi);
    std::vector<std::shared_ptr<e2sim::ue::UEInfo>> getUEs();

private:
    std::unordered_map<std::string, std::shared_ptr<e2sim::ue::UEInfo>> ue_map;  // IMSI, UE data

    std::mutex ue_lock; // prevents the multithreaded E2 and O1 interface to run into race conditions
};

class GlobalE2NodeData {
public:
    GlobalE2NodeData(std::string mcc, std::string mnc, uint32_t gnb_id);
    ~GlobalE2NodeData();

    GlobalE2node_ID_t *getGlobalE2NodeId();
    PLMN_Identity_t *getGlobalE2NodePlmnId();
    BIT_STRING_t *getGlobalE2Node_gNBId();

    bool addCell(std::shared_ptr<Cell> cell);
    void deleteCell(uint16_t pci);
    std::shared_ptr<Cell> getCell(uint16_t pci);
    void updateCellTxReferenceLevel(uint16_t pci, double gain);
    std::vector<std::shared_ptr<Cell>> getCells();

    UEList ue_list;

    const uint32_t gnbid;

    private:
    GlobalE2node_ID_t *globalE2NodeId;
    std::unordered_map<uint16_t, std::shared_ptr<Cell>> cells;
    std::mutex cellsLock;

};



#endif
