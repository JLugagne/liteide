#include "golangpls.h"

#include <QDebug>
#include "goplsserver.h"
#include "quickopenapi/quickopenapi.h"
#include "golangpls_global.h"

GolangPls::GolangPls(LiteApi::IApplication *app, QObject *parent)
    : QObject(parent)
    , m_liteApp(app)
    , m_completer(nullptr)
    , m_server(new GoPlsServer(app))
{
    LiteApi::IActionContext *actionContext = m_liteApp->actionManager()->getActionContext(this,"GolangPls");

    m_jumpDeclAct = new QAction(tr("Jump to Declaration"),this);
    actionContext->regAction(m_jumpDeclAct,"JumpToDeclaration!","CTRL+SHIFT+J;F2");

    connect(m_jumpDeclAct, &QAction::triggered, this, &GolangPls::editorJumpToDecl);

    connect(m_liteApp, &LiteApi::IApplication::sessionListChanged, this, &GolangPls::appLoaded);
    connect(m_server, &GoPlsServer::logMessage, this, &GolangPls::onLogMessage);
    connect(m_server, &GoPlsServer::autocompleteResult, this, &GolangPls::onAutoCompletion);
    connect(m_server, &GoPlsServer::definitionsResult, this, &GolangPls::onDefinitionResult);
    connect(m_server, &GoPlsServer::formattingResults, this, &GolangPls::onFormattingResults);
    connect(m_server, &GoPlsServer::updateFile, this, &GolangPls::onUpdateFile);
    connect(m_server, &GoPlsServer::hoverResult, this, &GolangPls::onHoverResult);
    connect(m_server, &GoPlsServer::hoverDefinitionResult, this, &GolangPls::onHoverDefinitionResult);
    connect(m_server, &GoPlsServer::diagnosticsInfo, this, &GolangPls::onDiagnosticsInfo);
    connect(m_server, &GoPlsServer::documentSymbolsResult, this, &GolangPls::onDocumentSymbolsResult);

    connect(m_liteApp->editorManager(),&LiteApi::IEditorManager::currentEditorChanged,this, &GolangPls::currentEditorChanged);
    connect(m_liteApp->editorManager(),&LiteApi::IEditorManager::editorCreated,this, &GolangPls::editorCreated);
    connect(m_liteApp->editorManager(), &LiteApi::IEditorManager::editorAboutToClose, this, &GolangPls::editorClosed);
    connect(m_liteApp->editorManager(), &LiteApi::IEditorManager::editorAboutToSave, this, &GolangPls::editorSaved);

    connect(m_liteApp->optionManager(),SIGNAL(applyOption(QString)),this,SLOT(applyOption(QString)));
    m_liteApp->optionManager()->emitApplyOption(OPTION_GOLANGPLS);
    //connect(m_liteApp->editorManager(), &LiteApi::IEditorManager::editorModifyChanged, this, &GolangPls::editorChanged);

    //connect(m_liteApp->optionManager(),&LiteApi::IOptionManager::applyOption,this, &GolangPls::applyOption);
}

GolangPls::~GolangPls()
{
    QEventLoop loop;
    connect(m_server, &GoPlsServer::exited, &loop, &QEventLoop::quit);
    m_server->shutdown();
    loop.exec();
    m_server->quit();
}

void GolangPls::setCompleter(LiteApi::ICompleter *completer)
{
    if (m_completer) {
        disconnect(m_completer,0,this,0);
    }
    m_completer = completer;
    if (m_completer) {
        m_completer->setSearchSeparator(false);
        m_completer->setExternalMode(true);
        connect(m_completer,SIGNAL(prefixChanged(QTextCursor,QString,bool)),this,SLOT(prefixChanged(QTextCursor,QString,bool)));
        connect(m_completer,SIGNAL(wordCompleted(QString,QString,QString)),this,SLOT(wordCompleted(QString,QString,QString)));
    }
}

void GolangPls::currentEditorChanged(LiteApi::IEditor *editor)
{
    if (!editor) {
        this->setCompleter(0);
        return;
    }

    auto liteEditor = LiteApi::getLiteEditor(editor);
    liteEditor->clearAnnotations("staticcheck");

    if (editor->mimeType() == "text/x-gosrc") {
        LiteApi::ICompleter *completer = LiteApi::findExtensionObject<LiteApi::ICompleter*>(editor,"LiteApi.ICompleter");
        this->setCompleter(completer);
    } else if (editor->mimeType() == "browser/goplay") {
        LiteApi::IEditor* pedit = LiteApi::findExtensionObject<LiteApi::IEditor*>(m_liteApp->extension(),"LiteApi.Goplay.IEditor");
        if (pedit && pedit->mimeType() == "text/x-gosrc") {
            editor = pedit;
            LiteApi::ICompleter *completer = LiteApi::findExtensionObject<LiteApi::ICompleter*>(editor,"LiteApi.ICompleter");
            this->setCompleter(completer);
        }
    } else {
        this->setCompleter(0);
        return;
    }


    m_editor = LiteApi::getTextEditor(editor);
    if (!m_editor) {
        return;
    }
    if(m_currentFile != "") {
        m_server->fileClosed(m_currentFile);
    }
    m_currentFile = m_editor->filePath();
    QPlainTextEdit *plainTextEditor = LiteApi::getPlainTextEdit(m_editor);
    m_server->fileOpened(m_currentFile, plainTextEditor->toPlainText());
    m_server->documentSymbols(editor->filePath());
    m_plainText = plainTextEditor->toPlainText();
}

void GolangPls::editorCreated(LiteApi::IEditor *editor)
{
    if (editor->mimeType() != "text/x-gosrc") {
        return;
    }

    //editor->widget()->addAction(m_commentAct);
    QMenu *menu = LiteApi::getEditMenu(editor);
    if (menu) {
        menu->addAction(m_jumpDeclAct);
    }

    menu = LiteApi::getContextMenu(editor);
    if (menu) {
        menu->addAction(m_jumpDeclAct);
    }

    QString filePath = editor->filePath();
    if (filePath.isEmpty()) {
        return;
    }
    auto workspaceDirectory = findModulePath(filePath);
    if(!m_opendWorkspace.contains(workspaceDirectory) && workspaceDirectory != "/") {
        m_server->addWorkspaceFolder(workspaceDirectory);
        m_opendWorkspace.insert(workspaceDirectory, true);
    }
    connect(editor, &LiteApi::IEditor::contentsChanged, this, &GolangPls::editorChanged);
    connect(editor, SIGNAL(updateLink(QTextCursor,QPoint,bool)), this, SLOT(onUpdateLink(QTextCursor,QPoint,bool)));

    QPlainTextEdit *plainTextEditor = LiteApi::getPlainTextEdit(editor);
    const QString filepath = editor->filePath();
    m_plainText = plainTextEditor->toPlainText();
    connect(plainTextEditor, &QPlainTextEdit::textChanged, [this, filepath, plainTextEditor]() {
        int startLine, startPos, endLine, endPos;
        QString content;
        computeModifications(m_plainText, plainTextEditor->toPlainText(), startLine, startPos, endLine, endPos, content);
        m_server->fileChanged(filepath, startLine, startPos, endLine, endPos, content);
        m_plainText = plainTextEditor->toPlainText();
        //m_server->documentHighlight(m_currentFile);
    });
}

void GolangPls::editorClosed(LiteApi::IEditor *editor)
{
    // editor is the new one, get the last one
    LiteApi::ITextEditor *currentEditor = LiteApi::getTextEditor(editor);
    if(currentEditor) {
        m_server->fileClosed(currentEditor->filePath());
    }
}

void GolangPls::editorSaved(LiteApi::IEditor *editor)
{
    LiteApi::ITextEditor *currentEditor = LiteApi::getTextEditor(editor);
    //    m_server->organizeImports(currentEditor->filePath());
    m_server->formatDocument(currentEditor->filePath());
    m_server->fileSaved(currentEditor->filePath(), LiteApi::getPlainTextEdit(currentEditor)->toPlainText());
    m_liteApp->editorManager()->saveEditor(editor,false);
}

void GolangPls::editorChanged()
{
    QObject *source = QObject::sender();
    LiteApi::IEditor *editor = dynamic_cast<LiteApi::IEditor *>(source);
    if(editor) {
        m_server->documentSymbols(editor->filePath());
    }
}

void GolangPls::prefixChanged(QTextCursor cur, QString pre, bool force)
{
    m_preWord.clear();
    if (pre.endsWith('.')) {
        m_preWord = pre;
    }
    if (!m_preWord.isEmpty()) {
        m_completer->clearItemChilds(m_preWord);
    }

    const QPoint pos = cursorPosition(cur);
    m_server->askAutocomplete(m_editor->filePath(), pos.y(), pos.x());
}

void GolangPls::wordCompleted(QString, QString, QString)
{
    m_preWord.clear();
}

void GolangPls::onLogMessage(const QString &message, bool isError)
{
    m_liteApp->appendLog("GolangPls", message, isError);
}

void GolangPls::onAutoCompletion(const QList<AutoCompletionResult> &result)
{
    QStandardItem *root= m_completer->findRoot(m_preWord);
    for(const auto &item: result) {
        m_completer->appendChildItem(root,item.label,item.type,item.detail,QIcon(),true);
    }
    m_completer->updateCompleterModel();
    m_completer->showPopup();
}

void GolangPls::onDefinitionResult(const QList<DefinitionResult> &definitions)
{
    if(definitions.isEmpty()){
        return;
    }

    DefinitionResult result = definitions.first();
    QString fileName = result.path;
    LiteApi::gotoLine(m_liteApp,fileName,result.line, result.column,true,true);
}

void GolangPls::onFormattingResults(const QList<TextEditResult> &list)
{
    editText(m_editor, list);

    m_server->organizeImports(m_currentFile);
}

void GolangPls::onUpdateFile(const QString &filename, const QList<TextEditResult> &list)
{
    LiteApi::IEditor *editor = m_liteApp->editorManager()->findEditor(filename,true);
    if (!editor) {
        QString mimeType = m_liteApp->mimeTypeManager()->findMimeTypeByFile(filename);
        editor = m_liteApp->editorManager()->openEditor(filename, mimeType);
    }

    if (!editor) {
        return;
    }

    editText(editor, list);
}

void GolangPls::onHoverResult(const QList<HoverResult> &result)
{
    for(auto &res : result) {
        m_lastLink.showTip = true;
        m_lastLink.targetInfo = res.info;
        fromLineAndColumnToPos(m_editor, res.startLine, res.startColumn, m_lastLink.linkTextStart);
        fromLineAndColumnToPos(m_editor, res.endLine, res.endColumn, m_lastLink.linkTextEnd);

        auto editor = LiteApi::getLiteEditor(m_editor);
        if(editor)
            editor->showLink(m_lastLink);
        break;
    }
}

void GolangPls::onHoverDefinitionResult(const QList<DefinitionResult> &definitions)
{
    for(auto def : definitions) {
        m_lastLink.targetFileName = def.path;
        m_lastLink.targetLine = def.line;
        m_lastLink.targetColumn = def.column;
        m_lastLink.showNav = true;

        auto editor = LiteApi::getLiteEditor(m_editor);
        if(editor)
            editor->showLink(m_lastLink);
        break;
    }
}

void GolangPls::onDiagnosticsInfo(const QString &filename, const QList<DiagnosticResult> &diagnostics)
{
    auto editor = m_liteApp->editorManager()->findEditor(filename, false);
    if(editor) {
        auto liteEditor = LiteApi::getLiteEditor(editor);
        if(liteEditor) {
            liteEditor->clearAnnotations("staticcheck");
            for(auto diag : diagnostics) {
                bool disabled = true;
                for(auto check : m_staticcheckEnables) {
                    if(diag.code.startsWith(check)) {
                        disabled = false;
                        break;
                    }
                }
                if(disabled) {
                    continue;
                }
                LiteApi::Annotation annotation;
                annotation.content = diag.message;
                annotation.level = diag.level;
                annotation.from = "staticcheck";
                liteEditor->addAnnotation(diag.line, annotation);
            }
        }
    }
}

void GolangPls::onDocumentSymbolsResult(const QString &filename, const QList<LiteApi::Symbol> &symbols)
{
    auto editor = m_liteApp->editorManager()->findEditor(filename, false);
    if(editor) {
        auto liteEditor = LiteApi::getLiteEditor(editor);
        if(liteEditor) {
            liteEditor->loadSymbols(symbols);
        }
    }
}

void GolangPls::editorJumpToDecl()
{
    if(!m_editor) {
        return;
    }

    const QPoint pos = cursorPosition(m_editor->textCursor());
    m_server->askDefinitions(m_editor->filePath(), false, pos.y(), pos.x());
}

void GolangPls::editorFindUsages()
{

}

void GolangPls::editText(LiteApi::IEditor *liteEditor, const QList<TextEditResult> &list, bool reverse)
{
    QByteArray state = liteEditor->saveState();
    auto editor = LiteApi::getPlainTextEdit(liteEditor);
    auto cursor = editor->textCursor();
    QString text = editor->toPlainText();
    QList<TextEditResult> edits;
    for(auto it = list.crbegin(); it != list.crend(); ++it) {
        auto changes = *it;
        int startPos;
        fromLineAndColumnToPos(liteEditor, changes.startLine, changes.startColumn, startPos);

        int endPos;
        fromLineAndColumnToPos(liteEditor, changes.endLine, changes.endColumn, endPos);

        cursor.setPosition(startPos);
        cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        cursor.insertText(changes.text);
    }
    liteEditor->restoreState(state);
}

void GolangPls::onUpdateLink(const QTextCursor &cursor, const QPoint &curPos, bool nav)
{
    auto editor = dynamic_cast<LiteApi::IEditor*>(sender());
    if(!editor) {
        return;
    }

    int pos = cursor.position();
    const QString text = LiteApi::getPlainTextEdit(editor)->toPlainText();
    int line = 0;
    int column = 0;
    fromPosToLineAndColumn(editor, pos, line, column);
    m_lastLink.clear();
    m_lastLink.cursorPos = curPos;
    m_server->hover(editor->filePath(), line, column);
    m_server->askDefinitions(editor->filePath(), true, line, column);
}

void GolangPls::computeModifications(const QString &original, const QString &current, int &startLine, int &startPos, int &endLine, int &endPos, QString &content)
{
    int start = 0;
    int end = 0;
    content = "";
    while(true) {
        if(start < original.length() && start < current.length()) {
            if(original.at(start) != current.at(start)) {
                break;
            }
            start++;
        }else{
            break;
        }
    }
    int fromEnd = 0;
    while(true) {
        if(original.length() - fromEnd > start && current.length() - fromEnd > start) {
            if(original.at(original.length()-fromEnd-1) != current.at(current.length()-fromEnd-1)) {
                break;
            }
            fromEnd++;
        }else{
            break;
        }
    }
    end = original.length()-fromEnd;
    content = current.mid(start, current.length()-fromEnd-start);
    const QString startString = original.left(start);
    const QString endString = original.left(end);
    startLine = startString.count("\n");
    startPos = start-startString.lastIndexOf("\n")-1;
    endLine = endString.count("\n");
    endPos = end-endString.lastIndexOf("\n")-1;
}

void GolangPls::fromLineAndColumnToPos(LiteApi::IEditor *editor, int line, int column, int &pos) const
{
    auto liteEditor = LiteApi::getLiteEditor(editor);
    if(liteEditor) {
        auto doc = liteEditor->document();
        auto block = doc->findBlockByLineNumber(line);
        pos = block.position()+column;
    }
}

void GolangPls::fromPosToLineAndColumn(LiteApi::IEditor *editor, int pos, int &line, int &column) const
{
    auto liteEditor = LiteApi::getLiteEditor(editor);
    if(liteEditor) {
        auto doc = liteEditor->document();
        auto block = doc->findBlock(pos);
        line = block.blockNumber();
        column = pos-block.position();
    }
}

void GolangPls::applyOption(QString id)
{
    if (id != OPTION_GOLANGPLS) {
        return;
    }

    bool sa1 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA1,true).toBool();
    bool sa2 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA2,true).toBool();
    bool sa3 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA3,true).toBool();
    bool sa4 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA4,true).toBool();
    bool sa5 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA5,true).toBool();
    bool sa6 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA6,true).toBool();
    bool sa9 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_SA9,true).toBool();
    bool s1 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_S1,true).toBool();
    bool st1 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_ST1,true).toBool();
    bool qf1 = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_QF1,true).toBool();
    bool unreachable = m_liteApp->settings()->value(GOLANGPLS_STATICCHECK_UNREACHABLE,true).toBool();

    m_staticcheckEnables.clear();
    if(sa1) {
        m_staticcheckEnables << "SA1";
    }
    if(sa2) {
        m_staticcheckEnables << "SA2";
    }
    if(sa3) {
        m_staticcheckEnables << "SA3";
    }
    if(sa4) {
        m_staticcheckEnables << "SA4";
    }
    if(sa5) {
        m_staticcheckEnables << "SA5";
    }
    if(sa6) {
        m_staticcheckEnables << "SA6";
    }
    if(sa9) {
        m_staticcheckEnables << "SA9";
    }
    if(s1) {
        m_staticcheckEnables << "S1";
    }
    if(st1) {
        m_staticcheckEnables << "ST1";
    }
    if(qf1) {
        m_staticcheckEnables << "QF1";
    }
    if(unreachable) {
        m_staticcheckEnables << "unreachable";
    }
}

void GolangPls::appLoaded()
{
    for(auto editor: m_liteApp->editorManager()->editorList()) {
        auto path = findModulePath(editor->filePath());
        if(path == "/") {
            continue;
        }
        m_opendWorkspace[path] = true;
    }

    m_server->initWorkspace(m_opendWorkspace.keys());
}

QString GolangPls::findModulePath(const QString &filepath) const
{
    QString workspaceDirectory = QFileInfo(filepath).absoluteDir().absolutePath();
    while(workspaceDirectory != "/") {
        if(QFile::exists(workspaceDirectory+"/go.mod")) {
            break;
        }
        workspaceDirectory = QFileInfo(workspaceDirectory).absoluteDir().absolutePath();
    }
    return workspaceDirectory;
}

QPoint GolangPls::cursorPosition(QTextCursor cur) const
{
    const int column = cur.columnNumber();
    cur.movePosition(QTextCursor::StartOfLine);

    int lines = 1;
    while(cur.positionInBlock()>0) {
        cur.movePosition(QTextCursor::Up);
        lines++;
    }
    QTextBlock block = cur.block().previous();

    while(block.isValid()) {
        lines += block.lineCount();
        block = block.previous();
    }
    return QPoint(column, lines-1);
}
