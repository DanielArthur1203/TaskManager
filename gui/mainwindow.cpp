#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processes.hpp"
#include <QMessageBox>
#include <QTableWidget>
#include <QScrollBar>
#include <QDebug>
#include <algorithm>
#include <cctype>
#include <iterator>
#include <future>
#include <thread>
#include <chrono>
#include <unordered_set>

using namespace std::chrono;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    ui->processesTable->setColumnCount(4);
    ui->processesTable->verticalHeader()->setVisible(false);
    ui->processesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->processesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers;
    //Network is too stoke inducing to do since accurate values require kernel level access
    headers << "Name" << "CPU" << "Memory" << "Disk";
    ui->processesTable->setHorizontalHeaderLabels(headers);
    refreshProcesses();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refreshProcesses);
    timer->start(1000);

    scrollTimer = new QTimer(this);
    scrollTimer->setInterval(150);
    scrollTimer->setSingleShot(true);
    connect(scrollTimer, &QTimer::timeout, this, &MainWindow::scrollStopped);

    connect(ui->processesTable->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleScroll);
    ui->processesTable->setColumnWidth(0, 150);
}

MainWindow::~MainWindow(){
    delete ui;
}

std::vector<std::string> MainWindow::getDifferences1(std::list<std::string>& names){
    std::vector<std::string> diff;

    for(const auto& name: names){
        if(map.find(name) == map.end()){
            diff.push_back(name);
        }
    }
    return diff;
}

std::vector<std::string> MainWindow::getDifferences2(std::list<std::string> &names){
    std::vector<std::string> diff;

    std::unordered_set<std::string> set(names.begin(), names.end());

    for(const auto& pair: map){
        if(set.find(pair.first) == set.end()){
            diff.push_back(pair.first);
        }
    }
    return diff;
}

void MainWindow::resolveDifferences(std::list<std::string>& names, Processes& p){
    auto diff1 = getDifferences1(names);
    auto diff2 = getDifferences2(names);

    //only need to add
    if(diff2.empty() && (!diff1.empty())){
        for(auto& name: diff1){
            populateMapHelper(name, p);
        }
    }
    //Only need to delete
    else if(diff1.empty() && (!diff2.empty())){
        for(auto& name: diff2){
            map.erase(name);
        }
    }
    else{//Need both
        for(auto& name: diff1){
            populateMapHelper(name, p);
        }
        for(auto& name: diff2){
            map.erase(name); 
        }
    }
}

void MainWindow::populateMapHelper(std::string &name, Processes &p){
    if(!name.contains(".exe")){
        //If it doesn't have an exe then getPIDFromName will probably fail in some way
        //So just putting -1 should be fine
        std::wstring passed(name.begin(), name.end());
        std::vector<DWORD> pids = p.getPIDFromName(passed);
        if(!pids.empty()){
            auto [iterator, inserted] = map.try_emplace(name, pids.at(0), true, pids);
        }
    }
    else{
        std::wstring passed(name.begin(), name.end());
        std::vector<DWORD> pids = p.getPIDFromName(passed);
        if(!pids.empty()){
            auto [iterator, inserted] = map.try_emplace(name, pids.at(0), false, pids);
        }
    }
}

void MainWindow::populateMap(std::list<std::string> &names, Processes& p){
    auto start = steady_clock::now();
    int count = 0;

    for(auto& name: names){
        populateMapHelper(name, p);
        count++;
    }
    auto end = steady_clock::now();
    duration<double> sec = end - start;
    qDebug() << sec << " seconds";
    qDebug() << count << " objects made";
}

void MainWindow::refreshProcesses(){
    Processes p;
    std::list<std::string> exeNames;

    try{
        exeNames = p.allProcessNamesWthExe();
        exeNames.sort();
        exeNames.unique();
    }
    catch(const std::runtime_error& e){
        QMessageBox::critical(this, "Error", e.what());
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Unexpected Error", e.what());
    }

    try{
        if(map.empty()){
            populateMap(exeNames, p);
        }
        else{
            resolveDifferences(exeNames, p);
            needNameChange = true;
        }
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Error:", e.what());
    }
    
    if(!map.empty()){
        try{
            ui->processesTable->setRowCount(map.size());
            int i = 0;
            for(const auto& pair: map){
                std::vector<QTableWidgetItem*> items;
                //Gotta loop through a vector or something of QTableWidgetItems
                QString a = QString::fromStdString(pair.first);
                QTableWidgetItem *displayedName = new QTableWidgetItem(a);
                items.push_back(displayedName);

                const auto& stats = pair.second;
                auto cpuUsage = stats.getCpuUsage();
                auto memoryUsage = stats.getMemoryUsage();
                auto diskUsage = stats.getDiskUsage();

                if(auto val = std::get_if<double>(&cpuUsage)){
                    auto item = new QTableWidgetItem(QString::number(*val) + QString("%"));
                    items.push_back(item);
                }
                else{
                    auto s = std::get<std::string>(cpuUsage);
                    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(s));
                    items.push_back(item);
                }

                if(auto val = std::get_if<double>(&memoryUsage)){
                    auto item = new QTableWidgetItem(QString::number(*val) + QString(" MB"));
                    items.push_back(item);
                }
                else{
                    auto s = std::get<std::string>(memoryUsage);
                    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(s));
                    items.push_back(item);
                }

                if(auto val = std::get_if<double>(&diskUsage)){
                    auto item = new QTableWidgetItem(QString::number(*val) + QString(" MB/s"));
                    items.push_back(item);
                }
                else{
                    auto s = std::get<std::string>(diskUsage);
                    QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(s));
                    items.push_back(item);
                }

                for(int k = 0; k < items.size(); ++k){
                    ui->processesTable->setItem(i, k, items.at(k));
                }
                i++;
            }
            needNameChange = true;
            //oldMap = std::move(map);
            //oldProcessList = std::move(exeNames);
        }
        catch(const std::runtime_error& e){
            QMessageBox::critical(this, "Error:", e.what());
        }
    }

    //don't loop through the whole table. use a stack or something to find what needs changing
    // if(needNameChange){
    //     int rowCount = ui->processesTable->rowCount();

    //     for(int i = 0; i < rowCount; i++){
    //         QTableWidgetItem* item = ui->processesTable->item(i, 0);

    //         if(item){
    //             QString exeName = item->text();
    //             std::wstring stringExeName = exeName.toStdWString();

    //             std::vector<DWORD> pids;
    //             std::wstring pid;
    //             std::wstring fdName;
    //             try{
    //                 pids = p.getPIDFromName(stringExeName);
    //                 pid = p.fullPathFromPID(pids.at(0));
    //                 fdName = p.fileDescriptorName(pid);
    //             }
    //             catch(const std::runtime_error& e){
    //                 std::string msg = e.what();
    //                 continue;
    //             }
    //             exeName = QString::fromStdWString(fdName);
    //             item->setText(exeName);
    //         }
    //     }
    //     needNameChange = false;
    // }
}

void MainWindow::scrollStopped(){
    timer->start();
}

void MainWindow::handleScroll(){
    if(timer->isActive()){
        timer->stop();
    }
    scrollTimer->start();
}
