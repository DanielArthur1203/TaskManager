#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processes.hpp"
#include <QMessageBox>
#include <QTableWidget>
#include <QScrollBar>
#include <algorithm>
#include <cctype>
#include <iterator>

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

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refreshProcesses);
    timer->start(1000);

    scrollTimer = new QTimer(this);
    scrollTimer->setInterval(150);
    scrollTimer->setSingleShot(true);
    connect(scrollTimer, &QTimer::timeout, this, &MainWindow::scrollStopped);

    connect(ui->processesTable->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handleScroll);
    ui->processesTable->setColumnWidth(0, 150); //Name
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
    else if(!(names == oldProcessList)){
        if(names.size() != oldProcessList.size()){
            ui->processesTable->setRowCount(names.size());
        }
        resolveDifferences(names);
        oldProcessList = names;
    }
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
