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
    HttpRequest(ui->Type->currentText().toStdString(), ui->URL->currentText().toStdString(), ui->RequestBody->toPlainText().toStdString(), getHeaders(ui->RequestHeaders->toPlainText()));
}

void MainWindow::OnRequest(const string& result, const string& headers)
{
    ui->ResponseBody->setPlainText(result.data());
    ui->ResponseHeaders->setPlainText(headers.data());

    bool found = false;
    QString ct = ui->URL->currentText();
    for (int i = 0; i < ui->URL->count(); i++)
    {
        if (ui->URL->itemText(i) == ct)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        ui->URL->addItem(ct);
        ui->URL->model()->sort(0);
    }
}

void MainWindow::on_URL_currentIndexChanged(int index)
{
    on_Send_clicked();
}
