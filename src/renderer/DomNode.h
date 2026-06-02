#ifndef DOMNODE_H
#define DOMNODE_H

#include <QString>
#include <QList>
#include <QDebug>
#include <utility>

class DomNode {
public:
    enum NodeType { Element, Text };

    NodeType type;
    QString tagName;
    QString textContent;
    QList<DomNode*> children;

    // Constructor
    DomNode(NodeType t) : type(t) {}

    // Destructor to clean up children recursively
    ~DomNode() {
        qDeleteAll(children);
    }

    // Prints the DOM tree structure to console
    void printTree(int depth = 0) {
        QString indent = QString(depth * 2, ' ');
        if (type == Element) {
            qDebug().noquote() << indent << "<" + tagName + ">";

            // UPDATE THIS LINE: Wrap 'children' in std::as_const()
            for (DomNode* child : std::as_const(children)) {
                child->printTree(depth + 1);
            }

            qDebug().noquote() << indent << "</" + tagName + ">";
        } else {
            qDebug().noquote() << indent << '"' + textContent + '"';
        }
    }
};

#endif // DOMNODE_H