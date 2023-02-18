#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl.hpp>
//#include <boost/asio/ssl/error.hpp>
//#include <boost/asio/ssl/stream.hpp>
#include <QMainWindow>

using namespace  std;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void OnRequest(const string& result, const string& headers = "");

public slots:
    void on_Send_clicked();

private:
    Ui::MainWindow *ui;
    void HttpRequest(const std::string&, const std::string&, const std::string&, const std::unordered_map<string,string>&);

};

#endif // MAINWINDOW_H
