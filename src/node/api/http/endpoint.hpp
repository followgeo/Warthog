#pragma once
#define UWS_NO_ZLIB
#include "api/types/all.hpp"
#include "block/block.hpp"
#include "general/tcp_util.hpp"
#include "uwebsockets/App.h"
#include <thread>
#include <variant>

using WebsocketEvent = std::variant<API::Block>;

struct Config;
class IndexGenerator {
public:
    void get(std::string s);
    void post(std::string s);
    void section(std::string s);
    std::string result(bool isPublic) const;

private:
    bool fresh{true};
    std::string inner;
};

class HTTPEndpoint {
public:
    static std::optional<HTTPEndpoint> make_public_endpoint(const Config&);
    HTTPEndpoint(EndpointAddress bind, bool isPublic = false);
    ~HTTPEndpoint()
    {
        lc.loop->defer(std::bind(&HTTPEndpoint::shutdown, this));
        t.join();
    }
    void push_event(WebsocketEvent e)
    {
        lc.loop->defer([this, e = std::move(e)]() mutable {
            on_event(std::move(e));
        });
    };

private:
    void async_reply(uWS::HttpResponse<false>* res, std::string reply)
    {
        lc.loop->defer(std::bind(&HTTPEndpoint::send_reply, this, res, std::move(reply)));
    }
    void work();
    void shutdown();
    void on_event(WebsocketEvent&& e);

    void send_reply(uWS::HttpResponse<false>* res, const std::string& s);
    void get(std::string pattern, auto asyncfun, auto serializer, bool priv = false);
    void get(std::string pattern, auto asyncfun, bool priv = false);
    void get_1(std::string pattern, auto asyncfun, bool priv = false);
    void get_2(std::string pattern, auto asyncfun, bool priv = false);
    void get_3(std::string pattern, auto asyncfun, bool priv = false);
    void post(std::string pattern, auto parser, auto asyncfun, bool priv = false);

    //////////////////////////////
    // handlers
    void on_aborted(uWS::HttpResponse<false>* res);
    void on_listen(us_listen_socket_t* ls);

    //////////////////////////////
    // handlers for websocket events
    void handle_event(const API::Block&);

    //////////////////////////////
    // variables
    IndexGenerator indexGenerator;
    std::set<uWS::HttpResponse<false>*> pendingRequests;
    EndpointAddress bind;
    bool isPublic;
    us_listen_socket_t* listen_socket = nullptr;
    const uWS::LoopCleaner lc;
    uWS::App app;
    bool bshutdown = false;
    std::thread t;
};
