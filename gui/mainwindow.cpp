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


using namespace std::chrono;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    ui->processesTable->setColumnCount(6);
    ui->processesTable->verticalHeader()->setVisible(false);
    ui->processesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->processesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers;
    headers << "Name" << "CPU" << "Memory" << "Disk" << "Network" << "GPU";
    ui->processesTable->setHorizontalHeaderLabels(headers);
    refreshProcesses();

    //Timer blows it up now
    // timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, this, &MainWindow::refreshProcesses);
    // timer->start(1000);

    // scrollTimer = new QTimer(this);
    // scrollTimer->setInterval(150);
    // scrollTimer->setSingleShot(true);
    // connect(scrollTimer, &QTimer::timeout, this, &MainWindow::scrollStopped);

    // connect(ui->processesTable->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleScroll);
    ui->processesTable->setColumnWidth(0, 150); //Name
}

MainWindow::~MainWindow(){
    for(auto& [name,stats]: map){
        stats.stopThreads();
    }
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

void MainWindow::resolveDifferences(std::list<std::string>& names){
    auto diff1 = getDifferences1(names);
    auto diff2 = getDifferences2(names);

    //Need to delete only
    if((!diff1.empty()) && diff2.empty()){
        for(const auto& diff: diff1){
            QString s = QString::fromStdString(diff);
            QList<QTableWidgetItem*> items = ui->processesTable->findItems(s, Qt::MatchExactly);

            auto item = items.first();
            int row = item->row();
            ui->processesTable->removeRow(row);
        }
    }
    //Need to add only
    else if(diff1.empty() && (!diff2.empty())){
        for(const auto& diff: diff2){
            QString s = QString::fromStdString(diff);
            int rowCount = ui->processesTable->rowCount();
            int insertIndex = rowCount;
            for(int i = 0; i < rowCount; i++){
                auto item = ui->processesTable->item(i, 0);

                if(item){
                    if(s.compare(item->text(), Qt::CaseInsensitive) < 0){
                        insertIndex = i;
                        break;
                    }
                }
            }
            ui->processesTable->insertRow(insertIndex);
            ui->processesTable->setItem(insertIndex, 0, new QTableWidgetItem(s));
        }   
    }
    //Need both
    else{
        //Delete first
        for(const auto& diff: diff1){
            QString s = QString::fromStdString(diff);
            QList<QTableWidgetItem*> items = ui->processesTable->findItems(s, Qt::MatchExactly);

            auto item = items.first();
            int row = item->row();
            ui->processesTable->removeRow(row);
        }
        //Then add
        for(const auto& diff: diff2){
            QString s = QString::fromStdString(diff);
            int rowCount = ui->processesTable->rowCount();
            int insertIndex = rowCount;
            for(int i = 0; i < rowCount; i++){
                auto item = ui->processesTable->item(i, 0);

                if(item){
                    if(s.compare(item->text(), Qt::CaseInsensitive) < 0){
                        insertIndex = i;
                        break;
                    }
                }
            }
            ui->processesTable->insertRow(insertIndex);
            ui->processesTable->setItem(insertIndex, 0, new QTableWidgetItem(s));
        }
    }   
}

void MainWindow::populateMapHelper(std::string &name, Processes& p){
    if(!name.contains(".exe")){
        //If it doesn't have an exe then getPIDFromName will probably fail in some way
        //So just putting -1 should be fine
        std::wstring passed(name.begin(), name.end());
        std::vector<DWORD> pids = p.getPIDFromName(passed);
        auto [iterator, inserted] = map.try_emplace(name, pids.at(0), true, pids);
        // UsageStats stat = UsageStats(-1, true);
        // map.emplace(std::make_pair(name, std::move(stat)));
    }
    else{
        std::wstring passed(name.begin(), name.end());
        std::vector<DWORD> pids = p.getPIDFromName(passed);
        auto [iterator, inserted] = map.try_emplace(name, pids.at(0), false, pids);

        //UsageStats stat = UsageStats(pids.at(0), false, pids);
        //map.emplace(std::make_pair(name, std::move(stat)));
    }
}

void MainWindow::populateMap(std::list<std::string> &names, Processes& p){
    auto start = steady_clock::now();
    std::list<std::future<void>> results;
    //Names must be exe names
    for(auto& name: names){
        //qDebug() << "Doing process " + name;
        populateMapHelper(name, p);
    }
    // for(auto& res: results){
    //     res.get();
    // }
    auto end = steady_clock::now();
    duration<double> sec = end - start;
    qDebug() << sec << " seconds"; //takes 3+ seconds
}

void MainWindow::refreshProcesses(){
    //IDK how to go about displaying the names without an exe since FD names are completly different compared to exe names
    Processes p;
    std::list<std::string> exeNames;
    //std::list<std::string> namesWithoutExe;
    std::future<std::list<std::string>> result;

    try{
        exeNames = p.allProcessNamesWthExe();
        // result = std::async(std::launch::async, [this, &p](){
        //     return p.allProcessesNames();
        // });
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
        //TODO Do something where is there is a difference in oldProcesses and exeNames do something to the map and not repopulate
        populateMap(exeNames, p);
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Error:", e.what());
    }
    
    if(oldMap.empty()){
        try{
            ui->processesTable->setRowCount(map.size());
            int i = 0;
            for(const auto& name: exeNames){
                std::vector<QTableWidgetItem*> items;
                //Gotta loop through a vector or something of QTableWidgetItems
                QString a = QString::fromStdString(name);
                QTableWidgetItem *displayedName = new QTableWidgetItem(a);
                items.push_back(displayedName);

                const auto& stats = map.at(name);
                auto cpuUsage = stats.getCpuUsage();
                auto memoryUsage = stats.getMemoryUsage();
                auto diskUsage = stats.getDiskUsage();
                //map.erase(name);//stats is gone now DO NOT REFERENCE IT

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
                    auto item = new QTableWidgetItem(QString::number(*val) + QString("%"));
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
                //map.at(name) = std::move(stats);
                i++;
            }
            oldMap = std::move(map);
        }
        catch(const std::runtime_error& e){
            QMessageBox::critical(this, "Error:", e.what());
        }
    }
    // if(oldProcessList.empty()){
    //     ui->processesTable->setRowCount(names.size());
    //     int i = 0;
    //     for(const auto& name: names){
    //         QString a = QString::fromStdString(name);
    //         QTableWidgetItem *item = new QTableWidgetItem(a);

    //         ui->processesTable->setItem(i, 0, item);
    //         i++;
    //     }
    //     oldProcessList = names;
    // }
    // else if(!(names == oldProcessList)){
    //     if(names.size() != oldProcessList.size()){
    //         ui->processesTable->setRowCount(names.size());
    //     }
    //     resolveDifferences(names);
    //     oldProcessList = names;
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
