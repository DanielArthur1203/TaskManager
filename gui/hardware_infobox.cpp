#include "hardware_infobox.hpp"

HardwareInfoBox::HardwareInfoBox(QFrame *parent, const QString& s): QTextEdit(parent), name(s){
    setReadOnly(true);
    setFrameStyle(QFrame::NoFrame);
    setTextInteractionFlags(Qt::NoTextInteraction);
    viewport()->setCursor(Qt::PointingHandCursor);
    setText(s);
    setObjectName(s);
}

void HardwareInfoBox::mouseReleaseEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        emit clicked(name);
    }
    QTextEdit::mouseReleaseEvent(event);
}
