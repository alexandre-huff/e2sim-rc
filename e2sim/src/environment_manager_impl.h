#ifndef __ENVIRONMENT_MANAGER_IMPL_H__
#define __ENVIRONMENT_MANAGER_IMPL_H__
#include <map>

struct ue_data {
	std::string imsi;
};

extern std::map<std::string, struct ue_data *> ue_map;

void start_environment_manager(uint16_t port);
void stop_environment_manager();

#endif /* __ENVIRONMENT_MANAGER_IMPL_H__ */
