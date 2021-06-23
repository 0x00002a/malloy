#pragma once

#include "malloy/http/request.hpp"
#include "malloy/http/generator.hpp"
#include "malloy/server/http/connection/connection_t.hpp"
#include <malloy/server/http/connection/detail/route_handoff.hpp>

#include "malloy/server/websocket/connection/connection_plain.hpp"
#if MALLOY_FEATURE_TLS
    #include "malloy/server/websocket/connection/connection_tls.hpp"
#endif

#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace spdlog
{
    class logger;
}

namespace malloy::server::http
{


    /**
     * An HTTP server connection.
     *
     * @note This uses CRTP to allow using the same code for different connection
     *       types (eg. plain or TLS).
     */
    template<class Derived>
    class connection
    {
    public:
        using handler_t = detail::route_handoff;

        /**
         * Session configuration structure.
         */
        struct config
        {
            std::uint64_t request_body_limit = 10 * 10e6;   ///< The maximum allowed body request size in bytes.
        };

        /**
         * The connection configuration.
         */
        struct config cfg;

        /**
         * Constructor
         *
         * @param logger
         * @param buffer
         * @param router
         * @param http_doc_root
         */
        connection(
            std::shared_ptr<spdlog::logger> logger,
            boost::beast::flat_buffer buffer,
            std::shared_ptr<handler_t> router,
            std::shared_ptr<const std::filesystem::path> http_doc_root
        ) :
            m_logger(std::move(logger)),
            m_buffer(std::move(buffer)),
            m_router(std::move(router)),
            m_doc_root(std::move(http_doc_root))
        {
            // Sanity check logger
            if (!m_logger)
                throw std::runtime_error("did not receive a valid logger instance.");

            // Sanity check router
            if (!m_router)
                throw std::runtime_error("did not receive a valid router instance.");
        }

        template<bool isRequest, class Body, class Fields>
        void
        do_write(boost::beast::http::message<isRequest, Body, Fields>&& msg)
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            m_response = sp;

            // Write the response
            boost::beast::http::async_write(
                derived().m_stream,
                *sp,
                boost::beast::bind_front_handler(
                    &connection::on_write,
                    derived().shared_from_this(),
                    sp->need_eof()
                )
            );
        }

        // ToDo: Should this be protected?
        void do_read()
        {
            m_logger->trace("do_read()");

            // Construct a new parser for each message
            m_parser.emplace();

            // Apply a reasonable limit to the allowed size
            // of the body in bytes to prevent abuse.
            m_parser->body_limit(cfg.request_body_limit);

            // Set the timeout.
            boost::beast::get_lowest_layer(derived().stream()).expires_after(std::chrono::seconds(30));

            // Read a request using the parser-oriented interface
            boost::beast::http::async_read(
                derived().m_stream,
                m_buffer,
                *m_parser,
                boost::beast::bind_front_handler(
                    &connection::on_read,
                    derived().shared_from_this()
                )
            );
        }

    protected:
        boost::beast::flat_buffer m_buffer;

    private:
        std::shared_ptr<spdlog::logger> m_logger;
        std::shared_ptr<const std::filesystem::path> m_doc_root;
        std::shared_ptr<handler_t> m_router;
        std::shared_ptr<void> m_response;

        // The parser is stored in an optional container so we can
        // construct it from scratch it at the beginning of each new message.
        boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_parser;

        /**
         * Cast to derived class type.
         *
         * @return Reference to the derived class type.
         */
        [[nodiscard]]
        Derived& derived()
        {
            return static_cast<Derived&>(*this);
        }

        void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            m_logger->trace("on_read(): bytes read: {}", bytes_transferred);

            // This means they closed the connection
            if (ec == boost::beast::http::error::end_of_stream)
                return do_close();

            // Check for errors
            if (ec) {
                m_logger->error("on_read(): {}", ec.message());
                return;
            }

            // Parse the request into something more useful from hereon
            malloy::http::request req = m_parser->release();

            // Check request URI for legality
            if (!req.uri().is_legal()) {
                m_logger->warn("illegal request URI: {}", req.uri().raw());
                auto resp = malloy::http::generator::bad_request("illegal URI");
                do_write(std::move(resp));
                return;
            }

            // Check if this is a WS request
            if (boost::beast::websocket::is_upgrade(req)) {
                m_logger->info("upgrading HTTP connection to WS connection.");

                // Create a websocket connection, transferring ownership
                // of both the socket and the HTTP request.
                auto ws_connection = server::websocket::make_websocket_connection(
                    m_logger->clone("websocket_connection"),
                    derived().release_stream()
                );

                // Launch the connection
                ws_connection->run(req);

                // Hand over to router
                m_router->websocket(*m_doc_root, std::move(req), ws_connection);
            }

            // This is an HTTP request
            else {
                // Hand over to router
                m_router->http(*m_doc_root, std::move(req), derived().shared_from_this());
            }
        }

        void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            m_logger->trace("on_write(): bytes written: {}", bytes_transferred);

            // Check for errors
            if (ec) {
                m_logger->error("on_write(): {}", ec.message());
                return;
            }

            if (close) {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                return do_close();
            }

            // We're done with the response so delete it
            m_response = { };

            // Read another request
            do_read();
        }

        /**
         * Close the connection.
         */
        void do_close()
        {
            m_logger->trace("do_close()");

            // Let the derived class handle this
            derived().do_close();

            // At this point the connection is closed gracefully
            m_logger->info("closed HTTP connection gracefully.");
        }
    };

}
