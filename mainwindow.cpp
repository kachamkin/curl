#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->Type->addItems({"GET", "POST", "PUT", "PATCH", "OPTIONS", "CONNECT", "HEAD", "TRACE", "DELETE"});


}

MainWindow::~MainWindow()
{
    delete ui;
}

unordered_map<string, string> getHeaders(const QString& headers)
{
    unordered_map<string, string> map;
    QStringList list = headers.split("\n");
    for (const QString& s : list)
    {
        if (!s.contains(":"))
            continue;
        QStringList h = s.split(":");
        map.insert({h[0].trimmed().toStdString(), h[1].trimmed().toStdString()});
    }
    return map;
}

void MainWindow::on_Send_clicked()
{
    ui->ResponseBody->setPlainText("");
    HttpRequest(ui->Type->currentText().toStdString(), ui->URL->text().toStdString(), ui->RequestBody->toPlainText().toStdString(), getHeaders(ui->RequestHeaders->toPlainText()));
}

void MainWindow::OnRequest(const string& result, const string& headers)
{
    ui->ResponseBody->setPlainText(result.data());
    ui->ResponseHeaders->setPlainText(headers.data());
}
