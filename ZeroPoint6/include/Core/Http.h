//
// Created by phosg on 3/25/2025.
//

#ifndef HTTP_H
#define HTTP_H

#include "Core/Types.h"
#include "Core/Vector.h"
#include "Core/Data.h"
#include "Core/String.h"

#define HTTP_STATUS_CODES(XX)                                                 \
    XX(100, CONTINUE,                        Continue)                        \
    XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
    XX(102, PROCESSING,                      Processing)                      \
    XX(200, OK,                              OK)                              \
    XX(201, CREATED,                         Created)                         \
    XX(202, ACCEPTED,                        Accepted)                        \
    XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
    XX(204, NO_CONTENT,                      No Content)                      \
    XX(205, RESET_CONTENT,                   Reset Content)                   \
    XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
    XX(207, MULTI_STATUS,                    Multi-Status)                    \
    XX(208, ALREADY_REPORTED,                Already Reported)                \
    XX(226, IM_USED,                         IM Used)                         \
    XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
    XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
    XX(302, FOUND,                           Found)                           \
    XX(303, SEE_OTHER,                       See Other)                       \
    XX(304, NOT_MODIFIED,                    Not Modified)                    \
    XX(305, USE_PROXY,                       Use Proxy)                       \
    XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
    XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
    XX(400, BAD_REQUEST,                     Bad Request)                     \
    XX(401, UNAUTHORIZED,                    Unauthorized)                    \
    XX(402, PAYMENT_REQUIRED,                Payment Required)                \
    XX(403, FORBIDDEN,                       Forbidden)                       \
    XX(404, NOT_FOUND,                       Not Found)                       \
    XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
    XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
    XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
    XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
    XX(409, CONFLICT,                        Conflict)                        \
    XX(410, GONE,                            Gone)                            \
    XX(411, LENGTH_REQUIRED,                 Length Required)                 \
    XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
    XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
    XX(414, URI_TOO_LONG,                    URI Too Long)                    \
    XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
    XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
    XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
    XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
    XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
    XX(423, LOCKED,                          Locked)                          \
    XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
    XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
    XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
    XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
    XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
    XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
    XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
    XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
    XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
    XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
    XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
    XX(508, LOOP_DETECTED,                   Loop Detected)                   \
    XX(510, NOT_EXTENDED,                    Not Extended)                    \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

#define HTTP_METHODS(XX)           \
    XX(GET,         GET)           \
    XX(HEAD,        HEAD)          \
    XX(OPTIONS,     OPTIONS)       \
    XX(TRACE,       TRACE)         \
    XX(PUT,         PUT)           \
    XX(DELETE,      DELETE)        \
    XX(POST,        POST)          \
    XX(PATCH,       PATCH)         \
    XX(CONNECT,     CONNECT)

#define HTTP_HEADERS(XX)                                        \
    XX(ACCEPT,                      Accept)                     \
    XX(ACCEPT_ENCODING,             Accept-Encoding)            \
    XX(CACHE_CONTROL,               Cache-Control)              \
    XX(CONNECTION,                  Connection)                 \
    XX(CONTENT_DIGEST,              Content-Digest)             \
    XX(CONTENT_ENCODING,            Content-Encoding)           \
    XX(CONTENT_LENGTH,              Content-Length)             \
    XX(CONTENT_TYPE,                Content-Type)               \
    XX(CONTENT_RANGE,               Content-Range)              \
    XX(COOKIE,                      Cookie)                     \
    XX(DATE,                        Date)                       \
    XX(HOST,                        Host)                       \
    XX(LOCATION,                    Location)                   \
    XX(LAST_MODIFIED,               Last-Modified)              \
    XX(SEC_WEBSOCKET_ACCEPT,        Sec-WebSocket-Accept)       \
    XX(SEC_WEBSOCKET_EXTENSIONS,    Sec-WebSocket-Extensions)   \
    XX(SEC_WEBSOCKET_KEY,           Sec-WebSocket-Key)          \
    XX(SEC_WEBSOCKET_PROTOCOL,      Sec-WebSocket-Protocol)     \
    XX(SEC_WEBSOCKET_VERSION,       Sec-WebSocket-Version)      \
    XX(SERVER,                      Server)                     \
    XX(UPGRADE,                     Upgrade)                    \
    XX(USER_AGENT,                  User-Agent)

namespace zp::Http
{
    enum StatusCode
    {
        ZP_HTTP_STATUS_UNKNOWN = 0,
#define XX(code, name, string)      ZP_HTTP_STATUS_##name = code,
        HTTP_STATUS_CODES( XX )
#undef XX
    };

    enum class Version
    {
        Unknown,
        Http1_0,
        Http1_1,
        Http2_0,
    };

    enum Method
    {
        ZP_HTTP_METHOD_UNKNOWN,
#define XX(name, string)        ZP_HTTP_METHOD_##name,
        HTTP_METHODS( XX )
#undef XX
    };

    enum HeaderKey
    {
        ZP_HTTP_HEADER_UNKNOWN,
#define XX(name, string)        ZP_HTTP_HEADER_##name,
        HTTP_HEADERS( XX )
#undef XX
    };

    struct Header
    {
        String value;
        HeaderKey key;
    };

    struct Request
    {
        String path;
        String body;
        FixedVector<Header, 16> headers;
        Version version;
        Method method;
        StatusCode statusCode;
    };

    struct Response
    {
        String path;
        String body;
        FixedVector<Header, 16> headers;
        Version version;
        StatusCode statusCode;
    };

    StatusCode ParseRequest( const Memory& inMemory, Request& outRequest );

    StatusCode WriteResponse( const Response& inResponse, DataStreamWriter& outMemory );
}
;

#endif //HTTP_H
