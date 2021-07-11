#include "../../test.hpp"

#include <malloy/client/controller.hpp>
#include <malloy/server/controller.hpp>
#include <malloy/server/routing/router.hpp>

namespace mc = malloy::client;
namespace ms = malloy::server;

TEST_SUITE("controller - client") {
    TEST_CASE("Client redirects to the new url when follow_redirect is true and it recieves a redirect status response") {
        ms::controller serve;
        mc::controller cli;

        constexpr auto interface = "127.0.0.1";
        constexpr uint16_t port = 12314;
        constexpr auto redir_resource = "/redir";
        constexpr auto actual_resource = "/actual";
        ms::controller::config cfg;
        cfg.interface = interface;
        cfg.port = port;
        cfg.logger = spdlog::default_logger();
        cfg.num_threads = 2;

        REQUIRE(serve.init(cfg));
        REQUIRE(cli.init(cfg));

        cli.set_follow_redirects(true);

        const auto router = serve.router();
        router->add_redirect(malloy::http::status::temporary_redirect, redir_resource, actual_resource);
        router->add(malloy::http::method::get, actual_resource, [&](auto&& req){
           CHECK(req.target() == actual_resource);
           return malloy::http::generator::ok();
        });

        malloy::http::request<> req{
            malloy::http::method::get,
            interface,
            port,
            redir_resource,
        };
        auto stop_tkn = cli.http_request(req, [](auto&& resp){
            CHECK(resp.result() == malloy::http::status::ok);
        });

        serve.start();
        cli.run();
        CHECK(!stop_tkn.get());
    }
}

