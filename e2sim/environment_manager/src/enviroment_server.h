#ifndef ENVIRONMENT_SERVER_H
#define ENVIRONMENT_SERVER_H
#include <vector>

#include "ApiBase.h"

using namespace org::openapitools::server::api;

void start_environment_server(uint16_t port, std::vector<std::shared_ptr<ApiBase>> &apiImpls);
void stop_environment_server();
#endif
