#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processes.hpp"
#include <QMessageBox>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace std::chrono;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    std::thread startUp(&MainWindow::refreshProcesses, this);
    std::this_thread::sleep_for(seconds(1));
    startUp.join();

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::refreshProcesses);
    timer->start(1000);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::refreshProcesses(){
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
    names.sort();
    if(oldProcessList.empty()){
        names.unique();
        for(const auto& name: names){
            QString q = QString::fromStdString(name);
            QListWidgetItem *item = new QListWidgetItem(q, ui->processesList);
            ui->processesList->setCurrentItem(item);
        }
        oldProcessList = names;
    }
    else{
        names.unique();
        if(!(std::is_permutation(names.begin(), names.end(), oldProcessList.begin(), oldProcessList.end()))){
            ui->processesList->clear();
            for(const auto& name: names){
                QString q = QString::fromStdString(name);
                QListWidgetItem *item = new QListWidgetItem(q, ui->processesList);
                ui->processesList->setCurrentItem(item);
            }
            oldProcessList = names;
        }
    }
}
