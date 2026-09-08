#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processes.hpp"
#include <QMessageBox>
#include <QTableWidget>
#include <algorithm>
#include <cctype>
#include <iterator>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    ui->processesTable->setColumnCount(6);
    ui->processesTable->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "Name" << "CPU" << "Memory" << "Disk" << "Network" << "GPU";
    ui->processesTable->setHorizontalHeaderLabels(headers);
    refreshProcesses();

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::refreshProcesses);
    timer->start(1000);

}

MainWindow::~MainWindow(){
    delete ui;
}

std::vector<std::string> MainWindow::getDifferences1(std::list<std::string>& names){
    std::vector<std::string> diff;

    std::set_difference(oldProcessList.begin(), oldProcessList.end(),
        names.begin(), names.end(), std::back_inserter(diff));
    return diff;
}

std::vector<std::string> MainWindow::getDifferences2(std::list<std::string> &names){
    std::vector<std::string> diff;

    std::set_difference(names.begin(), names.end(), oldProcessList.begin(), oldProcessList.end(), 
        std::back_inserter(diff));
    return diff;
}

void MainWindow::refreshProcesses(){
    //something here causes hitches while scrolling idk what
    //QCoreApplication::processEvents();
    Processes p;
    std::list<std::string> names;

    try{
        names = p.allProcessesNames();
    }
    catch(const std::runtime_error& e){
        QMessageBox::critical(this, "Error", e.what());
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Unexpected Error", e.what());
    }
    //A - Z ignoring case
    names.sort([](const std::string& a, const std::string& b){
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](unsigned char c1, unsigned char c2){
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    });
    names.unique();

    if(oldProcessList.empty()){
        ui->processesTable->setRowCount(names.size());
        int i = 0;
        for(const auto& name: names){
            QString a = QString::fromStdString(name);
            QTableWidgetItem *item = new QTableWidgetItem(a);

            ui->processesTable->setItem(i, 0, item);
            i++;
        }
        oldProcessList = names;
    }
    else if(!(std::is_permutation(names.begin(), names.end(), oldProcessList.begin(), oldProcessList.end()))){
        ui->processesTable->setRowCount(names.size());
        int i = 0;
        ui->processesTable->clearContents();
        for(const auto& name: names){
            QString a = QString::fromStdString(name);
            QTableWidgetItem *item = new QTableWidgetItem(a);

            ui->processesTable->setItem(i, 0, item);
            i++;
        }
        oldProcessList = names;
    }
}
