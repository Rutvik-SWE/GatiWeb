#ifndef HTMLPARSER_H
#define HTMLPARSER_H

#include <QString>
#include "DomNode.h"

class HtmlParser {
public:
    static DomNode* parseHtml(const QString &rawHtml);
};

#endif // HTMLPARSER_H