#include <memory>
#include <optional>
#include <string>

// Yay pistache
#ifdef DEBUG
#undef DEBUG
#endif

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include <api/TestingApi.h>
#include <api/ManagementApi.h>
#include <api/MonitoringApi.h>

#include <model/Ue_descriptor.h>
#include <model/_UE_get_200_response_inner.h>
#include <model/_UE__iMSI__anr_put_request.h>
#include <model/_UE__iMSI__flow_put_request.h>

#include "logger.h"

#include "environment_manager_impl.h"

#define PISTACHE_SERVER_THREADS	 2
#define PISTACHE_SERVER_MAX_REQUEST_SIZE 32768
#define PISTACHE_SERVER_MAX_RESPONSE_SIZE 32768

std::map<std::string, struct ue_data *> ue_map;
std::thread *pistache_server;

namespace org::openapitools::server::api {
using namespace org::openapitools::server::model;
class  TestingApiImpl : public org::openapitools::server::api::TestingApi {
public:
    explicit TestingApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr);
    ~TestingApiImpl() override = default;

    void test_get(Pistache::Http::ResponseWriter &response);

};

TestingApiImpl::TestingApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
    : TestingApi(rtr)
{ /* pass */ }

void TestingApiImpl::test_get(Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

class  ManagementApiImpl : public org::openapitools::server::api::ManagementApi {
public:
    explicit ManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr);
    ~ManagementApiImpl() override = default;

    void u_eimsi_admission_delete(const std::string &iMSI, Pistache::Http::ResponseWriter &response);
    void u_eimsi_admission_put(const std::string &iMSI, const Ue_descriptor &ueDescriptor, Pistache::Http::ResponseWriter &response);
    void u_eimsi_anr_put(const std::string &iMSI, const _UE__iMSI__anr_put_request &uEIMSIAnrPutRequest, Pistache::Http::ResponseWriter &response);
    void u_eimsi_flow_put(const std::string &iMSI, const _UE__iMSI__flow_put_request &uEIMSIFlowPutRequest, Pistache::Http::ResponseWriter &response);

};

ManagementApiImpl::ManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
    : ManagementApi(rtr)
{ /* pass */ }
void ManagementApiImpl::u_eimsi_admission_delete(const std::string &iMSI, Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

void ManagementApiImpl::u_eimsi_admission_put(const std::string &iMSI, const Ue_descriptor &ueDescriptor, Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

void ManagementApiImpl::u_eimsi_anr_put(const std::string &iMSI, const _UE__iMSI__anr_put_request &uEIMSIAnrPutRequest, Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

void ManagementApiImpl::u_eimsi_flow_put(const std::string &iMSI, const _UE__iMSI__flow_put_request &uEIMSIFlowPutRequest, Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

class  MonitoringApiImpl : public org::openapitools::server::api::MonitoringApi {
public:
    explicit MonitoringApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr);
    ~MonitoringApiImpl() override = default;

    void u_e_get(Pistache::Http::ResponseWriter &response);
    void u_eimsi_info_get(const std::string &iMSI, Pistache::Http::ResponseWriter &response);

};

MonitoringApiImpl::MonitoringApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
    : MonitoringApi(rtr)
{ /* pass */ }

void MonitoringApiImpl::u_e_get(Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

void MonitoringApiImpl::u_eimsi_info_get(const std::string &iMSI, Pistache::Http::ResponseWriter &response) {
    response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}



} //namespace org::openapitools::server::api


using namespace org::openapitools::server::api;

static Pistache::Http::Endpoint *httpEndpoint;

static void run_environment_manager(uint16_t port) {
	logger_info("Starting environment manager on port %d", port);
	Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(port));

	httpEndpoint = new Pistache::Http::Endpoint((addr));
	auto router = std::make_shared<Pistache::Rest::Router>();

	auto opts = Pistache::Http::Endpoint::options()
		.threads(PISTACHE_SERVER_THREADS);
	opts.flags(Pistache::Tcp::Options::ReuseAddr);
	opts.maxRequestSize(PISTACHE_SERVER_MAX_REQUEST_SIZE);
	opts.maxResponseSize(PISTACHE_SERVER_MAX_RESPONSE_SIZE);
	httpEndpoint->init(opts);
	auto apiImpls = std::vector<std::shared_ptr<ApiBase>>();

	apiImpls.push_back(std::make_shared<ManagementApiImpl>(router));
	apiImpls.push_back(std::make_shared<TestingApiImpl>(router));
	apiImpls.push_back(std::make_shared<MonitoringApiImpl>(router));

	for (auto api : apiImpls) {
		api->init();
	}

	httpEndpoint->setHandler(router->handler());
	logger_info("About to  call serve on enivronment manager");
	httpEndpoint->serve();
}

void start_environment_manager(uint16_t port)
{
	pistache_server = new std::thread(run_environment_manager, port);
}

void stop_environment_manager()
{
	httpEndpoint->shutdown();
}
