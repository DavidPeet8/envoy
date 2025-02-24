load("@envoy//bazel:envoy_build_system.bzl", "envoy_cc_library", "envoy_package", "envoy_select_envoy_mobile_request_compression", "envoy_select_google_grpc")

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "engine_builder_lib",
    srcs = [
        "engine_builder.cc",
    ],
    hdrs = [
        "engine_builder.h",
    ],
    copts = select({
        "//bazel:exclude_certificates": ["-DEXCLUDE_CERTIFICATES"],
        "//conditions:default": [],
    }),
    external_deps = ["abseil_optional"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":envoy_engine_cc_lib_no_stamp",
        "@envoy//source/common/common:assert_lib",
        "@envoy_api//envoy/config/bootstrap/v3:pkg_cc_proto",
        "@envoy_api//envoy/config/metrics/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/compression/brotli/decompressor/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/compression/gzip/decompressor/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/http/alternate_protocols_cache/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/http/decompressor/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/http/dynamic_forward_proxy/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/http/header_formatters/preserve_case/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/network/dns_resolver/apple/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/network/dns_resolver/getaddrinfo/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/transport_sockets/http_11_proxy/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/transport_sockets/quic/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/transport_sockets/raw_buffer/v3:pkg_cc_proto",
        "@envoy_mobile//library/common/config:certificates_lib",
        "@envoy_mobile//library/common/extensions/cert_validator/platform_bridge:platform_bridge_cc_proto",
        "@envoy_mobile//library/common/extensions/filters/http/local_error:filter_cc_proto",
        "@envoy_mobile//library/common/extensions/filters/http/network_configuration:filter_cc_proto",
        "@envoy_mobile//library/common/extensions/filters/http/socket_tag:filter_cc_proto",
        "@envoy_mobile//library/common/types:matcher_data_lib",
    ] + envoy_select_envoy_mobile_request_compression(
        [
            "@envoy_api//envoy/extensions/compression/brotli/compressor/v3:pkg_cc_proto",
            "@envoy_api//envoy/extensions/compression/gzip/compressor/v3:pkg_cc_proto",
            "@envoy_api//envoy/extensions/filters/http/composite/v3:pkg_cc_proto",
            "@envoy_api//envoy/extensions/filters/http/compressor/v3:pkg_cc_proto",
        ],
        "@envoy",
    ) + envoy_select_google_grpc(
        [
            "@envoy//source/common/grpc:google_grpc_creds_lib",
            "@envoy//source/extensions/config_subscription/grpc:grpc_collection_subscription_lib",
            "@envoy//source/extensions/config_subscription/grpc:grpc_subscription_lib",
            "@envoy//source/extensions/clusters/static:static_cluster_lib",
            "@envoy//source/extensions/clusters/strict_dns:strict_dns_cluster_lib",
            "@envoy//source/extensions/health_checkers/http:health_checker_lib",
        ],
        "@envoy",
    ),
)

envoy_cc_library(
    name = "envoy_engine_cc_lib_no_stamp",
    srcs = [
        "bridge_utility.cc",
        "engine.cc",
        "engine_callbacks.cc",
        "headers.cc",
        "headers_builder.cc",
        "key_value_store.cc",
        "log_level.cc",
        "request_headers.cc",
        "request_headers_builder.cc",
        "request_method.cc",
        "request_trailers.cc",
        "request_trailers_builder.cc",
        "response_headers.cc",
        "response_headers_builder.cc",
        "response_trailers.cc",
        "response_trailers_builder.cc",
        "retry_policy.cc",
        "stream.cc",
        "stream_callbacks.cc",
        "stream_client.cc",
        "stream_prototype.cc",
        "string_accessor.cc",
    ],
    hdrs = [
        "bridge_utility.h",
        "direct_response_testing.h",
        "engine.h",
        "engine_callbacks.h",
        "envoy_error.h",
        "headers.h",
        "headers_builder.h",
        "key_value_store.h",
        "log_level.h",
        "pulse_client.h",
        "request_headers.h",
        "request_headers_builder.h",
        "request_method.h",
        "request_trailers.h",
        "request_trailers_builder.h",
        "response_headers.h",
        "response_headers_builder.h",
        "response_trailers.h",
        "response_trailers_builder.h",
        "retry_policy.h",
        "stream.h",
        "stream_callbacks.h",
        "stream_client.h",
        "stream_prototype.h",
        "string_accessor.h",
        "trailers.h",
    ],
    external_deps = ["abseil_optional"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        "//library/common:envoy_main_interface_lib_no_stamp",
        "//library/common/api:c_types",
        "//library/common/data:utility_lib",
        "//library/common/extensions/key_value/platform:config",
        "@envoy//source/common/http:utility_lib",
    ],
)

envoy_cc_library(
    name = "envoy_engine_cc_lib",
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":engine_builder_lib",
        ":envoy_engine_cc_lib_no_stamp",
        "//library/common:engine_common_lib_stamped",
    ],
)
