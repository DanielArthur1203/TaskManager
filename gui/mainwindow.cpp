#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processes.hpp"
#include <QMessageBox>
#include <QTableWidget>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include <QTextEdit>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <future>
#include <thread>
#include <objbase.h>

struct Killer{
    ~Killer(){CoUninitialize();}
};

using namespace std::chrono;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    hardwareWatcher = new QFutureWatcher<HardwareInitResult>(this);

    connect(hardwareWatcher, &QFutureWatcher<HardwareInitResult>::finished,
        this, [this]{
            const auto res = hardwareWatcher->result();
            if(!res.err.isEmpty()){
                QMessageBox::critical(this, "Hardware info error", res.err);
                return; 
            }
            hardware = res.hardware;
            addTextBoxesToFrame();
        }
    );

    hardwareWatcher->setFuture(QtConcurrent::run([]{
        HardwareInitResult res;

        const HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if(FAILED(hres)){
            res.err = "COM init failed: " + QString::number(static_cast<unsigned long>(hres), 16);
            return res;
        }

        Killer k;

        try{
            res.hardware = std::make_shared<Hardware>();
        }
        catch(const std::exception& e){
            res.err = QString::fromLocal8Bit(e.what());
        }
        return res;
    }));
    ui->processesTable->setColumnCount(4);
    ui->processesTable->verticalHeader()->setVisible(false);
    ui->processesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->processesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers;
    //Network is too stroke inducing to do since accurate values require kernel level access
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

std::wstring MainWindow::displayNameRetrieval(DWORD pid, Processes &p){
    std::wstring fullPath = L"";
    try{
        fullPath = p.fullPathFromPID(pid);
    }
    catch(const std::runtime_error& e){
        std::string msg = e.what();
        if(msg.contains("5")){
            return L"PPL";
        }
        throw std::runtime_error(e.what());
    }
    catch(...){
        auto exp = std::current_exception();
        std::rethrow_exception(exp);
    }

    std::wstring displayName = L"";
    try{
        displayName = p.fileDescriptorName(fullPath);
    }
    catch(const std::runtime_error& e){
        std::string msg = e.what();
        if(msg.contains("5")){
            return L"PPL";
        }
        throw std::runtime_error(e.what());
    }
    catch(...){
        auto exp = std::current_exception();
        std::rethrow_exception(exp);
    }
    return displayName;
}

void MainWindow::addTextBoxesToFrame(){
    QWidget *page = ui->taskManagerStacked->widget(1);
    if(page){
        QFrame *frame = page->findChild<QFrame*>("hardwareButtonFrame");
        if(frame){
            QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(frame->layout());
            if(!layout){
                layout = new QVBoxLayout(frame);
                frame->setLayout(layout);
            }
            auto textBoxes = createTextBoxes(frame);
            int i = 0;
            //Cpu -> Memory ->Disks-> Wifi -> Gpu
            for(const auto& box: textBoxes){
                layout->addWidget(box);
                addPageToHardwareStackedWidget(ui->hardwareStackedWidget, box->objectName());
                connect(box, &HardwareInfoBox::clicked, this, &MainWindow::hardwareInfoBoxClicked);
                nameIndexMap.insert({box->objectName(), i});
                i++;
            }
        }
    }
}

std::vector<HardwareInfoBox *> MainWindow::createTextBoxes(QFrame* frame){
    std::vector<HardwareInfoBox *> textEdits;
    auto cpuText = makeTextBox(QString("CPU"), frame);
    textEdits.push_back(cpuText);

    auto memText = makeTextBox(QString("Memory"), frame);
    textEdits.push_back(memText);

    for(const auto& disk: hardware->disks){
        std::wstring name = disk->getName();
        name.erase(2);
        std::wstring info = L"Disk " + std::to_wstring(disk->getDiskNumber()) + L"(" + name + L")";
        auto text = makeTextBox(QString::fromStdWString(info), frame);
        textEdits.push_back(text);
    }

    //Need to make Ethernet or other stuff say that name eventually
    int j = 0;
    for(const auto& wifi: hardware->wifi){
        std::string info = "Wifi " + std::to_string(j);
        auto text = makeTextBox(QString::fromStdString(info), frame);
        textEdits.push_back(text);
        j++;
    }

    int i = 0;
    for(const auto& gpu: hardware->gpus){
        std::string a = "GPU " + std::to_string(i);
        auto text = makeTextBox(QString::fromStdString(a), frame);
        textEdits.push_back(text);
        i++;
    }
    return textEdits;
}

HardwareInfoBox *MainWindow::makeTextBox(const QString &s, QFrame* frame){
    auto text = new HardwareInfoBox(frame, s);
    return text;
}

void MainWindow::addPageToHardwareStackedWidget(QStackedWidget *widget, const QString& s){
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    auto text = new QTextEdit();
    text->setText(handleTextDeduction(s));
    text->setReadOnly(true);
    text->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(text);
    page->setLayout(layout);
    widget->addWidget(page);
}

QString MainWindow::handleTextDeduction(const QString &s){
    QString text("");
    //IDK how to force the names to the far right
    if(s.contains("CPU")){
        std::string a = "CPU\t\t\t" + hardware->cpu.getName();
        text = QString::fromStdString(a);
    }
    else if(s.contains("Memory")){
        std::wstring a = L"Memory\t\t\t" + hardware->memory.getName();
        text = QString::fromStdWString(a);
    }
    else if(s.contains("Disk")){
        QRegularExpression ex("Disk\\s+(\\d+)");
        QRegularExpressionMatch match = ex.match(s);

        if(match.hasMatch()){
            QString cap = match.captured(1);
            int index = cap.toInt();
            std::wstring a = s.toStdWString() + L"\t\t\t" + hardware->disks.at(index)->getManufacturerName();
            text = QString::fromStdWString(a);
        }
    }
    else if(s.contains("Wifi")){
        QStringList list = s.split(u' ', Qt::SkipEmptyParts);
        QString a = list.back();
        int index = a.toInt();

        std::wstring info = s.toStdWString() + L"\t\t\t" + hardware->wifi.at(index)->getAdapterName();
        text = QString::fromStdWString(info);
    }
    else{ //Can only be GPU left
        QStringList list = s.split(u' ', Qt::SkipEmptyParts);
        QString a = list.back();
        int index = a.toInt();

        std::wstring info = s.toStdWString() + L"\t\t\t" + hardware->gpus.at(index)->getName();
        text = QString::fromStdWString(info);
    }

    return text;
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
        QApplication::quit();
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Unexpected Error", e.what());
        QApplication::quit();
    }

    try{
        if(map.empty()){
            populateMap(exeNames, p);
        }
        else{
            resolveDifferences(exeNames, p);
        }
    }
    catch(const std::exception& e){
        QMessageBox::critical(this, "Error:", e.what());
        QApplication::quit();
    }
    
    if(!map.empty()){
        try{
            ui->processesTable->setRowCount(map.size());
            int i = 0;
            for(const auto& pair: map){
                std::vector<QTableWidgetItem*> items;
                QString a = "";
                //Messes with sorting but is a pain to fix(need to change difference retrieval)
                try{
                    a = QString::fromStdWString(displayNameRetrieval(pair.second.getPid(), p));
                }
                catch(const std::runtime_error& e){
                    a = QString::fromStdString(pair.first);
                }
                catch(...){
                    QMessageBox::critical(this, "Unkown Error:", "Unexpected Error Occurred During Display Name Retrieval");
                    QApplication::quit();
                }
                
                if(a.contains("PPL")){
                    a = QString::fromStdString(pair.first);
                }
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
        }
        catch(const std::runtime_error& e){
            QMessageBox::critical(this, "Error:", e.what());
            QApplication::quit();
        }
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

void MainWindow::on_swapScreenButton1_clicked(){
    ui->taskManagerStacked->setCurrentWidget(ui->page_2);
}

void MainWindow::on_swapScreenButton2_clicked(){
    ui->taskManagerStacked->setCurrentWidget(ui->page);
}

void MainWindow::hardwareInfoBoxClicked(QString &s){
    int currentPage = ui->hardwareStackedWidget->currentIndex();

    auto pair = nameIndexMap.find(s);
    if(pair == nameIndexMap.end()){
        QMessageBox::critical(this, "Error", "Unexpected Logic Error"); //Should never happen so idk how to describe it
        QApplication::quit();
    }
    
    int indexToGo = pair->second;

    if(indexToGo != currentPage){ //if the stacked widget is already on that page do nothing
        ui->hardwareStackedWidget->setCurrentIndex(indexToGo);
    }
}
