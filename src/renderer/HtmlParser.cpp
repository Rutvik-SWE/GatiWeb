#include "HtmlParser.h"
#include <QRegularExpression>
#include <QStack>

DomNode* HtmlParser::parseHtml(const QString &rawHtml) {
    // Clean out scripts and styles to avoid parsing layout clutter
    QString processedHtml = rawHtml;
    processedHtml.remove(QRegularExpression("<style[^>]*>.*?</style>", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption));
    processedHtml.remove(QRegularExpression("<script[^>]*>.*?</script>", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption));

    // Initialize root of our DOM tree
    DomNode* root = new DomNode(DomNode::Element);
    root->tagName = "document";

    // Stack tracking parent/child hierarchy layout
    QStack<DomNode*> stack;
    stack.push(root);

    QString currentText = "";
    QString currentTag = "";
    bool inTag = false;

    for (int i = 0; i < processedHtml.length(); ++i) {
        QChar c = processedHtml[i];

        if (c == '<') {
            // Found a tag. Process and save any plain text read before this point
            QString cleanText = currentText.simplified();
            if (!cleanText.isEmpty() && !stack.isEmpty()) {
                DomNode* textNode = new DomNode(DomNode::Text);
                textNode->textContent = cleanText;
                stack.top()->children.append(textNode);
            }
            currentText.clear();
            inTag = true;
            currentTag.clear();

        } else if (c == '>') {
            inTag = false;
            currentTag = currentTag.trimmed();

            if (currentTag.startsWith('/')) {
                // Closing tag encountered. Move back up one level.
                if (stack.size() > 1) {
                    stack.pop();
                }
            } else {
                // Opening tag encountered. Separate attributes from tag identifier name.
                QString tagName = currentTag.split(' ').first();

                DomNode* elementNode = new DomNode(DomNode::Element);
                elementNode->tagName = tagName;

                if (!stack.isEmpty()) {
                    stack.top()->children.append(elementNode);
                }

                // Ignore empty elements or singletons that cannot contain child elements
                if (!tagName.toLower().startsWith("img") &&
                    !tagName.toLower().startsWith("br") &&
                    !tagName.toLower().startsWith("meta") &&
                    !tagName.toLower().startsWith("link") &&
                    !currentTag.endsWith('/')) {
                    stack.push(elementNode);
                }
            }
        } else {
            if (inTag) {
                currentTag.append(c);
            } else {
                currentText.append(c);
            }
        }
    }

    return root;
}