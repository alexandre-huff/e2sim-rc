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

#include <TestingApi.h>
#include <E2NodeManagementApi.h>
#include <UEManagementApi.h>

#include "_E2Node_get_200_response.h"
#include "_UE__IMSI__put_request.h"
#include "_UEs_get_200_response.h"

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

	class  E2NodeManagementApiImpl : public org::openapitools::server::api::E2NodeManagementApi {
	public:
		explicit E2NodeManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr);
		~E2NodeManagementApiImpl() override = default;

		void e2_node_get(Pistache::Http::ResponseWriter &response);

	};

	class  UEManagementApiImpl : public org::openapitools::server::api::UEManagementApi {
	public:
		explicit UEManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr);
		~UEManagementApiImpl() override = default;

		void u_eimsi_delete(const std::string &iMSI, Pistache::Http::ResponseWriter &response);
		void u_eimsi_put(const std::string &iMSI, const _UE__IMSI__put_request &uEIMSIPutRequest, Pistache::Http::ResponseWriter &response);
		void u_es_get(Pistache::Http::ResponseWriter &response);

	};


	TestingApiImpl::TestingApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
		: TestingApi(rtr)
	{
	}

	void TestingApiImpl::test_get(Pistache::Http::ResponseWriter &response) {
		logger_info("envman: test API invoked");
		response.send(Pistache::Http::Code::Ok, "API reachable\n");
	}

	E2NodeManagementApiImpl::E2NodeManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
		: E2NodeManagementApi(rtr)
	{
	}

	void E2NodeManagementApiImpl::e2_node_get(Pistache::Http::ResponseWriter &response) {
		response.send(Pistache::Http::Code::Ok, "Do some magic\n");
	}

	UEManagementApiImpl::UEManagementApiImpl(const std::shared_ptr<Pistache::Rest::Router>& rtr)
		: UEManagementApi(rtr)
	{
	}

	void UEManagementApiImpl::u_eimsi_delete(const std::string &iMSI, Pistache::Http::ResponseWriter &response) {
		response.send(Pistache::Http::Code::Ok, "Do some magic\n");
	}
	void UEManagementApiImpl::u_eimsi_put(const std::string &iMSI,
			const _UE__IMSI__put_request &uEIMSIPutRequest,
			Pistache::Http::ResponseWriter &response) {
		struct ue_data *ue_data;
		auto iterator = ue_map.find(iMSI);
		if (iterator == ue_map.end())
			ue_data = (struct ue_data *)malloc(sizeof(struct ue_data));
		else
			ue_data = iterator->second;

		ue_data->imsi = iMSI;

		ue_map.insert_or_assign(iMSI, ue_data);

		response.send(Pistache::Http::Code::Ok, "Do some magic\n");
	}

	void UEManagementApiImpl::u_es_get(Pistache::Http::ResponseWriter &response) {
		response.send(Pistache::Http::Code::Ok, "Do some magic\n");
	}

}

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

	apiImpls.push_back(std::make_shared<E2NodeManagementApiImpl>(router));
	apiImpls.push_back(std::make_shared<TestingApiImpl>(router));
	apiImpls.push_back(std::make_shared<UEManagementApiImpl>(router));

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
