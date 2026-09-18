#pragma once

#include "usagestats.hpp"
#include <QMainWindow>
#include <QTimer>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private:
        Ui::MainWindow *ui;
        QTimer *timer;
        QTimer *scrollTimer;
        std::list<std::string> oldProcessList;
        std::map<std::string, UsageStats> map;
        std::map<std::string, UsageStats> oldMap;
        //Gets whats in oldProcessList but not names
        std::vector<std::string> getDifferences1(std::list<std::string>& names);
        //Get whats in names but not oldProcessList
        std::vector<std::string> getDifferences2(std::list<std::string>& names);
        void resolveDifferences(std::list<std::string>& names);
        void populateMapHelper(std::string& name, Processes& p);
        void populateMap(std::list<std::string>& names, Processes& p);
    private slots:
        void refreshProcesses();
        void scrollStopped();
        void handleScroll();
};
