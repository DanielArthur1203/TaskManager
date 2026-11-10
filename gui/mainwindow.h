#pragma once

#include "usagestats.hpp"
#include "hardware_infobox.hpp"
#include "../general/memory.hpp"
#include "../general/disk_factory.hpp"
#include "../general/wifi_factory.hpp"
#include "../general/gpu_factory.hpp"
#include <../general/cpu.hpp>
#include <QMainWindow>
#include <QTimer>
#include <QFutureWatcher>
#include <QString>
#include <QStackedWidget>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <cctype>
#include <array>

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

struct Hardware{
    GpuFactory gpuFac;
    DiskFactory diskFac;
    WifiFactory wifiFac;

    Cpu cpu;
    MemoryInfo memory;
    std::vector<std::unique_ptr<Disk>> disks = diskFac.makeDisks();
    std::vector<std::unique_ptr<GPU>> gpus = gpuFac.makeGPUs();
    std::vector<std::unique_ptr<WiFi>> wifi = wifiFac.makeWiFi();
};

struct HardwareInitResult{
    std::shared_ptr<Hardware> hardware;
    QString err;

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
        QFutureWatcher<HardwareInitResult>* hardwareWatcher = nullptr;
        std::shared_ptr<Hardware> hardware;
        std::map<std::string, UsageStats, Comparison> map;
        std::unordered_map<QString, int> nameIndexMap;
        //Gets what's new
        std::vector<std::string> getDifferences1(std::list<std::string>& names);
        //Gets what is old and should be deleted
        std::vector<std::string> getDifferences2(std::list<std::string>& names);
        void resolveDifferences(std::list<std::string>& names, Processes& p);
        void populateMapHelper(std::string& name, Processes& p);
        void populateMap(std::list<std::string>& names, Processes& p);
        std::wstring displayNameRetrieval(DWORD pid, Processes& p);
        void addTextBoxesToFrame();
        std::vector<HardwareInfoBox*> createTextBoxes(QFrame* frame);
        HardwareInfoBox* makeTextBox(const QString& s, QFrame* frame);
        void addPageToHardwareStackedWidget(QStackedWidget* widget, const QString& s);
        std::array<QString, 2> handleTextDeduction(const QString& s);
    private slots:
        void refreshProcesses();
        void scrollStopped();
        void handleScroll();
        void on_swapScreenButton1_clicked();
        void on_swapScreenButton2_clicked();
        void hardwareInfoBoxClicked(QString& s);
};
