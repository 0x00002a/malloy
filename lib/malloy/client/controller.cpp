#include "controller.hpp"
#include "http/connection_plain.hpp"

#if MALLOY_FEATURE_TLS
    #include "http/connection_tls.hpp"

    #include <boost/beast/ssl.hpp>
    #include <boost/certify/extensions.hpp>
    #include <boost/certify/https_verification.hpp>
#endif

using namespace malloy::client;

/**
 * Issue an HTTP request.
 *
 * @note We don't place this in the header to keep compilation times low.
 *
 * @tparam Connection The type of connection to use.
 * @param req The request to perform.
 * @return The corresponding response.
 */
template<class Connection>
[[nodiscard]]
static
std::future<malloy::http::response>
http_request_(malloy::http::request req, std::shared_ptr<Connection> connection)
{
    return std::async(
        std::launch::async,
        [req = std::move(req), conn = std::move(connection)] {
            malloy::http::response ret_resp;
            std::atomic_bool done = false;      // ToDo: Use std::atomic_flag instead

            // Launch
            conn->run(
                std::to_string(req.port()).c_str(),
                req,
                [&ret_resp, &done](malloy::http::response&& resp){
                    ret_resp = std::move(resp);
                    done = true;
                }
            );

            while (!done.load()) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }

            return ret_resp;
        }
    );
}

#if MALLOY_FEATURE_TLS
    bool controller::init_tls()
    {
        m_tls_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
        m_tls_ctx->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
        m_tls_ctx->set_default_verify_paths();
        boost::certify::enable_native_https_server_verification(*m_tls_ctx);

        return true;
    }
#endif // MALLOY_FEATURE_TLS

std::future<malloy::http::response>
controller::http_request_plain(malloy::http::request req)
{
    // Create connection
    auto conn = std::make_shared<http::connection_plain>(
        m_cfg.logger->clone(m_cfg.logger->name() + " | HTTP connection"),
        io_ctx()
    );

    // Perform request
    return http_request_(std::move(req), std::move(conn));
}

#if MALLOY_FEATURE_TLS
    std::future<malloy::http::response>
    controller::http_request_tls(malloy::http::request req)
    {
        // Create connection
        auto conn = std::make_shared<http::connection_tls>(
            m_cfg.logger->clone(m_cfg.logger->name() + " | HTTPS connection"),
            io_ctx(),
            *m_tls_ctx
        );

        // Perform request
        return http_request_(std::move(req), std::move(conn));
    }
#endif // MALLOY_FEATURE_TLS
