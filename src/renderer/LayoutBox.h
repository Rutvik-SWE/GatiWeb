#ifndef LAYOUTBOX_H
#define LAYOUTBOX_H

#include <QString>
#include <QList>
#include <QRect> // Provides X, Y, Width, and Height

class LayoutBox {
public:
    QString tagName;
    QString textContent;

    // The physical bounding box on the screen
    QRect geometry;

    QList<LayoutBox*> children;

    LayoutBox() {}

    ~LayoutBox() {
        qDeleteAll(children);
    }

    // Helper to print layout coordinates to the console
    void printLayout(int depth = 0) {
        QString indent = QString(depth * 2, ' ');
        QString rectInfo = QString("[X:%1 Y:%2 W:%3 H:%4]")
                               .arg(geometry.x())
                               .arg(geometry.y())
                               .arg(geometry.width())
                               .arg(geometry.height());

        if (textContent.isEmpty()) {
            qDebug().noquote() << indent << "<" + tagName + ">" << rectInfo;
        } else {
            qDebug().noquote() << indent << '"' + textContent + '"' << rectInfo;
        }

        for (LayoutBox* child : std::as_const(children)) {
            child->printLayout(depth + 1);
        }
    }
};

#endif // LAYOUTBOX_H