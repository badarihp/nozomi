#pragma once

namespace sakura {

template<> inline
RouteParamType to_RouteParamType<int>() { return RouteParamType::Int64; }

template<> inline
RouteParamType to_RouteParamType<folly::Optional<int>>() { return RouteParamType::OptionalInt64; }

template<> inline
RouteParamType to_RouteParamType<double>() { return RouteParamType::Double; }

template<> inline
RouteParamType to_RouteParamType<folly::Optional<double>>() { return RouteParamType::OptionalDouble; }

template<> inline
RouteParamType to_RouteParamType<std::string>() { return RouteParamType::String; }

template<> inline
RouteParamType to_RouteParamType<folly::Optional<std::string>>() { return RouteParamType::OptionalString; }

}
