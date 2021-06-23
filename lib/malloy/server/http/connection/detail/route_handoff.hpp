#pragma once 


#include <memory>
#include <filesystem>

#include <malloy/http/request.hpp>
#include <malloy/server/http/connection/connection_t.hpp>

namespace malloy::server {
	class router;

	namespace http::detail {
		/// This is needed to break the dependency cycle between connection and router
		class route_handoff {
		public:
			using request = malloy::http::request;
			using conn_t = const connection_t&;
			using path = std::filesystem::path;

			virtual void websocket(const path& root, request&& req, conn_t) = 0;
			virtual void http(const path& root, request&& req, conn_t) = 0;
		};
		auto router_handoff_adaptor(const std::shared_ptr<router>& router) -> std::unique_ptr<route_handoff>;
	}
}

