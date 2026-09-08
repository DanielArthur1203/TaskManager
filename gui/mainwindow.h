#pragma once

#include <QMainWindow>
#include <QTimer>
#include <list>
#include <string>
#include <vector>

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
        std::list<std::string> oldProcessList;

        //Gets whats in oldProcessList but not names
        std::vector<std::string> getDifferences1(std::list<std::string>& names);
        //Get whats in names but not oldProcessList
        std::vector<std::string> getDifferences2(std::list<std::string>& names);
    private slots:
        void refreshProcesses();
};
