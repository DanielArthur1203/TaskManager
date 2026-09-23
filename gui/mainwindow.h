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
#include <algorithm>
#include <cctype>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

struct Comparison{
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return std::lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end(),
            [](unsigned char c1, unsigned char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    }
};

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
        std::map<std::string, UsageStats, Comparison> map;
        //std::map<std::string, UsageStats, Comparison> oldMap;
        //Gets what's new
        std::vector<std::string> getDifferences1(std::list<std::string>& names);
        //Gets what is old and should be deleted
        std::vector<std::string> getDifferences2(std::list<std::string>& names);
        void resolveDifferences(std::list<std::string>& names, Processes& p);
        void populateMapHelper(std::string& name, Processes& p);
        void populateMap(std::list<std::string>& names, Processes& p);
    private slots:
        void refreshProcesses();
        void scrollStopped();
        void handleScroll();
};
