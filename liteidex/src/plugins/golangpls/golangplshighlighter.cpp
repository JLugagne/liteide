#include "golangplshighlighter.h"
#include <golangpls.h>

GolangPlsHighlighter::GolangPlsHighlighter(LiteApi::ITextEditor *editor, QTextDocument *document)
    : TextEditor::SyntaxHighlighter(document)
    , m_editor(editor)
{
    Q_UNUSED(editor)
}

TextEditor::SyntaxHighlighter::TextFormatId GolangPlsHighlighter::plsToFormat(unsigned int tokenType)
{
    switch (tokenType) {
        case 0: //namespace = 'namespace',
            return DataType;
        case 1: //type = 'type',
            return DataType;
        case 2: //class = 'class',
            return DataType;
        case 3: //enum = 'enum',
            return DataType;
        case 4: //interface = 'interface',
            return DataType;
        case 5: //struct = 'struct',
            return DataType;
        case 6: //typeParameter = 'typeParameter',
            return DataType;
        case 7: //parameter = 'parameter',
            return String;
        case 8: //variable = 'variable',
            return Normal;
        case 9: //property = 'property',
            return Normal;
        case 10: //enumMember = 'enumMember',
            return Error;
        case 11: //event = 'event',
            return Alert;
        case 12: //function = 'function',
            return Function;
        case 13: //method = 'method',
            return Function;
        case 14: //macro = 'macro',
            return Keyword;
        case 15: //keyword = 'keyword',
            return Keyword;
        case 16: //modifier = 'modifier',
            return Normal;
        case 17: //comment = 'comment',
            return Comment;
        case 18: //string = 'string',
            return String;
        case 19: //number = 'number',
            return Float;
        case 20: //regexp = 'regexp',
            return Normal;
        case 21: //operator = 'operator'
            return RegionMarker;
        default:
            return Normal;
    }
}


void GolangPlsHighlighter::highlightBlock(const QTextBlock &block)
{
    const auto highlights = m_highlights[block.blockNumber()];
    qDebug() << "=>" << block.text().replace(" ", "");
    for (const auto highlight : highlights) {
        setFormat(highlight.pos, highlight.length, m_creatorFormats[highlight.format]);
        qDebug() << block.blockNumber() << "set format" << highlight.pos << highlight.length << highlight.format;
        qDebug() << block.text().mid(highlight.pos, highlight.length) << "=" << highlight.format;
    }
}

void GolangPlsHighlighter::onHighlightResults(const QVariantList &list)
{
    qDebug() << list;
    m_highlights.clear();
    unsigned int previousLine = 0, previousColumn = 0;
    for (int i = 0; i < list.size(); i += 5) {
        unsigned int line = list[i].toUInt();
        unsigned int column = list[i + 1].toUInt();
        unsigned int length = list[i + 2].toUInt();
        unsigned int tokenType = list[i + 3].toUInt();
        unsigned int modifier = list[i + 4].toUInt();
        qDebug() << "line=" << line << ", column=" << column << ", length=" << length << modifier;
        if (line == 0) {
            column += previousColumn;
            line = previousLine;
        } else {
            line += previousLine;
        }
        qDebug() << " >> line=" << line << ", column=" << column << ", length=" << length;
        Highlight highlight;
        highlight.pos = column;
        highlight.length = length;
        highlight.format = plsToFormat(tokenType);
        m_highlights[line] << highlight;

        previousColumn = column;
        if (line > 0) {
            previousLine = line;
        }
    }
    rehighlight();
}
