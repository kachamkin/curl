#include "mainwindow.h"

class Url
{
public:
    string Host;
    string Schema;
    string Target;
    string Port;

    Url() : Url("") {}

    Url(string url)
    {
        Parse(url);
    }

    void Parse(string url)
    {
        if (url.empty())
            return;

        if (url.substr(0, 8) == "https://")
            Schema = "https";
        else if (url.substr(0, 7) == "http://")
            Schema = "http";
        else
            throw "Unknown schema";

        url = url.substr(Schema == "http" ? 7 : 8);

        size_t pos = url.find("/");
        if (pos == string::npos)
        {
            Target = "/";
            Host = url;
        }
        else
        {
            Target = url.substr(pos + 1);
            Host = url.substr(0, pos);
        }

        pos = Host.rfind(":");
        if (pos == string::npos)
        {
            if (Schema == "http")
                Port = "80";
            else
                Port = "443";
        }
        else
        {
            Port = url.substr(pos + 1);
            int port;
            try
            {
                port = stoi(Port);
            }
            catch (...)
            {
                throw "Invalid port";
            }

            if (!port || port >= 65536)
                throw string("Invalid port");

            Target = Target.substr(0, Host.rfind(":") + 1);
            Host = Host.substr(0, pos);
        }
    }

    static http::verb getType(const string& type)
    {
        if (type == "GET")
            return http::verb::get;
        if (type == "POST")
            return http::verb::post;
        if (type == "PUT")
            return http::verb::put;
        if (type == "DELETE")
            return http::verb::delete_;
        if (type == "OPTIONS")
            return http::verb::options;
        if (type == "HEAD")
            return http::verb::head;
        if (type == "TRACE")
            return http::verb::trace;
        if (type == "PATCH")
            return http::verb::patch;
        if (type == "CONNECT")
            return http::verb::connect;
        return http::verb::unknown;
    }
};

class ssl_session : public std::enable_shared_from_this<ssl_session>
{
    tcp::resolver resolver_;
    ssl::stream<tcp::socket> stream_;
    boost::beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

public:
    string result;
    string res_headers;
    // Resolver and stream require an io_context
    explicit
    ssl_session(boost::asio::io_context& ioc, ssl::context& ctx)
        : resolver_(ioc)
        , stream_(ioc, ctx)
    {
    }

    void
    fail(beast::error_code ec, char const* what)
    {
        result = string(what) + ": " + ec.message() + "\n";
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* target,
        int version, const string& type, const string& body, const std::unordered_map<string,string>& headers)
    {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream_.native_handle(), host))
        {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }

        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(Url::getType(type));
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req_.set(http::field::body, body);

        for (auto const& p : headers)
            req_.set(p.first, p.second);

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &ssl_session::on_resolve,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_resolve(
        boost::system::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Make the connection on the IP address we get from a lookup
        boost::asio::async_connect(
            stream_.next_layer(),
            results.begin(),
            results.end(),
            std::bind(
                &ssl_session::on_connect,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_connect(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "connect");

        // Perform the SSL handshake
        stream_.async_handshake(
            ssl::stream_base::client,
            std::bind(
                &ssl_session::on_handshake,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_handshake(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");

        // Send the HTTP request to the remote host
        http::async_write(stream_, req_,
            std::bind(
                &ssl_session::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Receive the HTTP response
        http::async_read(stream_, buffer_, res_,
            std::bind(
                &ssl_session::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Write the message to standard out
       result = res_.body();
       for (auto const& h : res_.base())
            res_headers += h.name_string().to_string() + ": " + h.value().to_string() + "\n";

        // Gracefully close the stream
        stream_.async_shutdown(
            std::bind(
                &ssl_session::on_shutdown,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_shutdown(boost::system::error_code ec)
    {
        if(ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }
        if(ec)
            return;// fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
};


// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

    void
    fail(beast::error_code ec, char const* what)
    {
        result = string(what) + ": " + ec.message() + "\n";
    }


public:
    string result;
    string res_headers;
    // Objects are constructed with a strand to
    // ensure that handlers do not execute concurrently.
    explicit
    session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , stream_(net::make_strand(ioc))
    {

    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* target,
        int version, const string& type, const string& body, const std::unordered_map<string,string>& headers)
    {
        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(Url::getType(type));
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req_.set(http::field::body, body);

        for (auto const& p : headers)
            req_.set(p.first, p.second);


        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set a timeout on the operation
        stream_.expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        stream_.async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
            return fail(ec, "connect");

        // Set a timeout on the operation
        stream_.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        http::async_write(stream_, req_,
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Receive the HTTP response
        http::async_read(stream_, buffer_, res_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Write the message to standard out
        //std::cout << res_ << std::endl;
        result = res_.body();
        for (auto const& h : res_.base())
            res_headers += h.name_string().to_string() + ": " + h.value().to_string() + "\n";

        // Gracefully close the socket
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
};

void MainWindow::HttpRequest(const std::string& type, const std::string& url, const std::string& body, const std::unordered_map<string,string>& headers)
{

    Url u;
    try
    {
        u.Parse(url);
    }
    catch (...)
    {
       OnRequest("Failed to parse URL");
       return;
    };

    // The io_context is required for all I/O
        net::io_context ioc;

        // Launch the asynchronous operation

        if (u.Schema == "http")
        {
        shared_ptr<session> ptr = std::make_shared<session>(ioc);
        ptr->run(u.Host.data(), u.Port.data(), u.Target.data(), 11, type, body, headers);

        // Run the I/O service. The call will return when
        // the get operation is complete.
        ioc.run();
        OnRequest(ptr->result, ptr->res_headers);
        }
        else
        {
            ssl::context ctx{ssl::context::sslv23_client};

                // This holds the root certificate used for verification
            //load_root_certificates(ctx);

            shared_ptr<ssl_session> ptr = std::make_shared<ssl_session>(ioc, ctx);
            ptr->run(u.Host.data(), u.Port.data(), u.Target.data(), 11, type, body, headers);

            // Run the I/O service. The call will return when
            // the get operation is complete.
            ioc.run();
            OnRequest(ptr->result, ptr->res_headers);

        }
}
