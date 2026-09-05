#pragma once

#include <QMainWindow>
#include <QTimer>
#include <list>
#include <string>

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
    private slots:
        void refreshProcesses();
};
