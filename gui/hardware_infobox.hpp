#ifndef HARDWARE_INFOBOX
#define HARDWARE_INFOBOX

#include <QFrame>
#include <QTextEdit>
#include <QMouseEvent>
#include <QString>

class HardwareInfoBox: public QTextEdit{
    Q_OBJECT

    public:
        explicit HardwareInfoBox(QFrame* parent, const QString& s);
    
    signals:
        void clicked(QString& s);
    
    protected:
        void mouseReleaseEvent(QMouseEvent* event);

    private:
        QString name;
};

#endif