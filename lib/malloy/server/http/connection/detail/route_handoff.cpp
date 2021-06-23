#include "route_handoff.hpp"

#include "malloy/server/routing/router.hpp"

namespace malloy::server::http::detail {

class router_adaptor: public route_handoff {
    using router_t = std::shared_ptr<malloy::server::router>;
public:
    using conn_t = const connection_t&;

    router_adaptor(router_t router) : router_{std::move(router)} {}

    void websocket(const std::filesystem::path& root, malloy::http::request&& req, conn_t conn) override { 
        router_->handle_request<true>(root, std::move(req), conn); 
    }
    void http(const std::filesystem::path& root, malloy::http::request&& req, conn_t conn) override { 
        router_->handle_request<false>(root, std::move(req), conn); 
    }
private:
    router_t router_;
};
auto router_handoff_adaptor(const std::shared_ptr<router>& router) -> std::unique_ptr<route_handoff> {
    return std::make_unique<router_adaptor>(router);
}
}
