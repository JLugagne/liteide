#ifndef GOLANGPLS_H
#define GOLANGPLS_H

#include <QObject>
#include <QTemporaryFile>

#include "liteapi/liteapi.h"
#include "liteeditorapi/liteeditorapi.h"
#include "processex/processex.h"
#include "goplstypes.h"

/**
 * init pls on first editor changed if go.mod found
 * wait for initialized before doing anything
 */

class GoPlsServer;
class GolangPls : public QObject
{
    Q_OBJECT
public:
    explicit GolangPls(LiteApi::IApplication *app, QObject *parent = 0);
    ~GolangPls();

    void setCompleter(LiteApi::ICompleter *completer);

public slots:
    void currentEditorChanged(LiteApi::IEditor*editor);
    void editorCreated(LiteApi::IEditor *editor);
    void editorClosed(LiteApi::IEditor *editor);
    void editorSaved(LiteApi::IEditor *editor);
    void editorChanged();

    void prefixChanged(QTextCursor, QString, bool force);
    void wordCompleted(QString,QString,QString);

    void onLogMessage(const QString &message, bool isError);
    void onAutoCompletion(const QList<AutoCompletionResult> &result);
    void onDefinitionResult(const QList<DefinitionResult> &definitions);
    void onFormattingResults(const QList<TextEditResult> &list);
    void onUpdateFile(const QString &filename, const QList<TextEditResult> &list);
    void onHoverResult(const QList<HoverResult> &result);
    void onHoverDefinitionResult(const QList<DefinitionResult> &definitions);
    void onDiagnosticsInfo(const QString &filename, const QList<DiagnosticResult> &diagnostics);

    void editorJumpToDecl();
    void editorFindUsages();
    void editText(LiteApi::IEditor *editor, const QList<TextEditResult> &list, bool reverse = false);
    void onUpdateLink(const QTextCursor &cursor, const QPoint &pos, bool nav);

    static void computeModifications(const QString &original, const QString &current, int &startLine, int &startPos, int &endLine, int &endPos, QString &content);
    static void fromLineAndColumnToPos(const QString &text, int line, int column, int &pos);
    static void fromPosToLineAndColumn(const QString &text, int pos, int &line, int &column);
signals:

private:
    QPoint cursorPosition(QTextCursor cur) const;

    LiteApi::IApplication *m_liteApp;
    LiteApi::ITextEditor  *m_editor;
    LiteApi::ICompleter   *m_completer;

    GoPlsServer *m_server;
    // contains all the go.mod files added to pls server
    QHash<QString, bool> m_opendWorkspace;

    QString m_preWord;

    QAction *m_jumpDeclAct;
    QString m_plainText;
    QString m_currentFile;

    QTextCursor m_linkCursor;
    LiteApi::Link m_lastLink;
};

#endif // GOLANGPLS_H
