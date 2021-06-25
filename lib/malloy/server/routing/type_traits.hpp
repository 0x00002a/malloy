#pragma once

#include <concepts> 
#include <variant>


namespace malloy::server::concepts {
namespace detail {
    struct any_callable {
            template<typename T>
            void operator()(T&&) {}
    };
}

template<typename V>
concept is_variant = requires(V v) { 
    std::visit(detail::any_callable{}, v); 
};

template<typename H>
concept advanced_route_handler = std::invocable<H, typename H::request_type> &&
    requires(H handler, const typename H::request_type::header_type& h)
{
    {
        handler.body_for(h)
        } -> is_variant;
};
} // namespace malloy::server::concepts