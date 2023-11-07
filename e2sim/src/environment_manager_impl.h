#ifndef __ENVIRONMENT_MANAGER_IMPL_H__
#define __ENVIRONMENT_MANAGER_IMPL_H__
#include <map>

class anr_entry {
public:
	std::string bbu_name;
	double rsrp;
	double rsrq;
	double sinr;
	double cqi;
	double bler;
};

class flow_entry {
public:
	double average_throughput;
	double latency;
};


class ue_data {
public:
	std::string imsi;
	flow_entry flow;
	std::map<std::string, std::shared_ptr<anr_entry>> anr;
	std::string endpoint;
};

extern std::map<std::string, std::shared_ptr<ue_data>> ue_map;

void start_environment_manager(uint16_t port);
void stop_environment_manager();

#endif /* __ENVIRONMENT_MANAGER_IMPL_H__ */
