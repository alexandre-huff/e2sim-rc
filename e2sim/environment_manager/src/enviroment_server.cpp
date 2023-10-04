#include <thread>
#include <cassert>

#include "pistache/endpoint.h"
#include "pistache/http.h"
#include "pistache/router.h"

#include "enviroment_server.h"

using namespace org::openapitools::server::api;

static Pistache::Http::Endpoint *httpEndpoint;

static void serve() {
	httpEndpoint->serve();
}

void start_environment_server(uint16_t port, std::vector<std::shared_ptr<ApiBase>> &apiImpls) {
	assert(port > 0);
	assert(apiImpls.size() > 0);

	Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(port));

	httpEndpoint = new Pistache::Http::Endpoint(addr);
	auto router = std::make_shared<Pistache::Rest::Router>();

	auto opts = Pistache::Http::Endpoint::options().threads(2);
	opts.flags(Pistache::Tcp::Options::ReuseAddr);
	opts.maxRequestSize(32768);
	opts.maxResponseSize(32768);
	httpEndpoint->init(opts);

	for (auto api : apiImpls) {
		api->init();
	}

	httpEndpoint->setHandler(router->handler());
	std::thread th(serve);
}

void stop_environment_server() {
	httpEndpoint->shutdown();
}
