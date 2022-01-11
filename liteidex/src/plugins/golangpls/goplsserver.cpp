#include "goplsserver.h"

#include "generated.h"
#include <QFileInfo>
#include <QTimer>
#include "liteenvapi/liteenvapi.h"

#define DECODE_CALLBACK(fn) std::bind(fn, this, _1, _2)
using namespace GoPlsTypes;
using namespace std::placeholders;

GoPlsServer::GoPlsServer(LiteApi::IApplication *app, QObject *parent)
    : QObject(parent)
    , m_init(false)
{

    m_envManager = LiteApi::getEnvManager(app);
    m_process = new QProcess(this);

    connect(m_envManager, &LiteApi::IEnvManager::currentEnvChanged, this, &GoPlsServer::currentEnvChanged);
    m_process->setProgram("gopls");
    m_process->setArguments({"-mode=stdio"});
    connect(m_process, &QProcess::started, this, &GoPlsServer::onStarted);
    connect(m_process, &QProcess::readyRead, this, &GoPlsServer::onReadyRead);
    m_process->start();
}

GoPlsServer::~GoPlsServer()
{
    m_process->kill();
    m_process->deleteLater();
}

void GoPlsServer::quit()
{
    m_process->kill();
}

void GoPlsServer::initWorkspace(const QStringList &_folders)
{
    QSharedPointer<InitializeParams> params(new InitializeParams());
    ClientCapabilities *clientCapabilites = new ClientCapabilities();
    TextDocumentClientCapabilities *documentCapabilities = new TextDocumentClientCapabilities();
    PublishDiagnosticsClientCapabilities *publishCapabilities = new PublishDiagnosticsClientCapabilities();
    publishCapabilities->setCodeDescriptionSupport(new bool(true));
    publishCapabilities->setDataSupport(new bool(true));
    publishCapabilities->setRelatedInformation(new bool(true));
    publishCapabilities->setVersionSupport(new bool(true));
    documentCapabilities->setPublishDiagnostics(publishCapabilities);
    DocumentFormattingClientCapabilities *formattingCapabilities = new DocumentFormattingClientCapabilities;
    formattingCapabilities->setDynamicRegistration(new bool(true));
    documentCapabilities->setFormatting(formattingCapabilities);
    DocumentRangeFormattingClientCapabilities *rangeFormattingCapabilites = new DocumentRangeFormattingClientCapabilities;
    rangeFormattingCapabilites->setDynamicRegistration(new bool(true));
    documentCapabilities->setRangeFormatting(rangeFormattingCapabilites);
    DocumentColorClientCapabilities *colorCapabilities = new DocumentColorClientCapabilities;
    colorCapabilities->setDynamicRegistration(new bool(true));
    documentCapabilities->setColorProvider(colorCapabilities);
    CompletionTextDocumentClientCapabilities *completionCapabilities = new CompletionTextDocumentClientCapabilities();
    CompletionTextDocumentClientCapabilitiesItem *completionCapabilitiesItem = new CompletionTextDocumentClientCapabilitiesItem();
    completionCapabilitiesItem->setCommitCharactersSupport(new bool(true));
    completionCapabilitiesItem->setInsertReplaceSupport(new bool(true));
    completionCapabilitiesItem->setSnippetSupport(new bool(true));
    completionCapabilities->setCompletionItem(completionCapabilitiesItem);
    documentCapabilities->setCompletion(completionCapabilities);
    TextDocumentSyncClientCapabilities *documentClientCapabilities = new TextDocumentSyncClientCapabilities();
    documentClientCapabilities->setDynamicRegistration(new bool(true));
    documentClientCapabilities->setWillSave(new bool(true));
    documentClientCapabilities->setWillSaveWaitUntil(new bool(true));
    documentClientCapabilities->setDidSave(new bool(true));
    documentCapabilities->setSynchronization(documentClientCapabilities);
    clientCapabilites->setTextDocument(documentCapabilities);
    params->setCapabilities(clientCapabilites);
    documentClientCapabilities->setWillSaveWaitUntil(new bool(true));
    documentClientCapabilities->setWillSave(new bool(true));
    WorkspaceClientCapabilities *workspaceCapabilities = new WorkspaceClientCapabilities();
    workspaceCapabilities->setWorkspaceFolders(new bool(true));
    DidChangeConfigurationWorkspaceClientCapabilities *changeWorkspaceCapabilites = new DidChangeConfigurationWorkspaceClientCapabilities;
    changeWorkspaceCapabilites->setDynamicRegistration(new bool(true));
    workspaceCapabilities->setDidChangeConfiguration(changeWorkspaceCapabilites);
    SemanticTokensWorkspaceClientCapabilities *workspaceSemanticTokens = new SemanticTokensWorkspaceClientCapabilities;
    workspaceSemanticTokens->setRefreshSupport(new bool(true));
    workspaceCapabilities->setSemanticTokens(workspaceSemanticTokens);
    clientCapabilites->setWorkspace(workspaceCapabilities);
    PublishDiagnosticsClientCapabilities *diagnostics = new PublishDiagnosticsClientCapabilities();
    diagnostics->setCodeDescriptionSupport(new bool(true));
    diagnostics->setDataSupport(new bool(true));
    diagnostics->setRelatedInformation(new bool(true));
    diagnostics->setVersionSupport(new bool(true));
    documentCapabilities->setPublishDiagnostics(diagnostics);
    QList<WorkspaceFolder *> *folders = new QList<WorkspaceFolder *>();
    for (auto &f : _folders) {
        auto folder = new WorkspaceFolder();
        folder->setUri(new DocumentURI("file://" + f));
        folders->append(folder);
    }
    //params->setRootUri(new DocumentURI("file://"+wFolder));
    params->setWorkspaceFolders(folders);

    SemanticTokensClientCapabilities *semanticTokens = new SemanticTokensClientCapabilities;
    documentCapabilities->setSemanticTokens(semanticTokens);
    //semanticTokens->setDynamicRegistration(new bool(true));
    semanticTokens->setMultilineTokenSupport(new bool(false));
    semanticTokens->setOverlappingTokenSupport(new bool(false));
    QList<QString *> *tokenTypes = new QList<QString *>();
    *tokenTypes << new QString("type")
                << new QString("class")
                << new QString("enum")
                << new QString("interface")
                << new QString("struct")
                << new QString("typeParameter")
                << new QString("parameter")
                << new QString("variable")
                << new QString("property")
                << new QString("enumMember")
                << new QString("event")
                << new QString("function")
                << new QString("method")
                << new QString("macro")
                << new QString("keyword")
                << new QString("modifier")
                << new QString("comment")
                << new QString("string")
                << new QString("number")
                << new QString("regexp")
                << new QString("operator")
                ;
    semanticTokens->setTokenTypes(tokenTypes);


    QList<QString *> *tokenModifiers = new QList<QString *>();
    *tokenModifiers << new QString("declaration")
                    << new QString("definition")
                    << new QString("readonly")
                    << new QString("static")
                    << new QString("deprecated")
                    << new QString("abstract")
                    << new QString("async")
                    << new QString("modification")
                    << new QString("documentation")
                    << new QString("defaultLibrary");
    semanticTokens->setTokenModifiers(tokenModifiers);

    QList<TokenFormat *> *tokenFormats = new QList<TokenFormat *>();
    *tokenFormats << new TokenFormat(TokenFormatRelative);
    semanticTokens->setFormats(tokenFormats);

    SemanticTokensWorkspaceClientCapabilitiesRequests *semanticCapabilities = new SemanticTokensWorkspaceClientCapabilitiesRequests;
    semanticCapabilities->setRange(new bool(false));
    semanticTokens->setRequests(semanticCapabilities);

    auto list = m_envManager->currentEnvironment().toStringList();
    QJsonObject env;
    for (const QString &var : list) {
        auto pos = var.indexOf("=");
        const QString name = var.left(pos);
        const QString value = var.mid(pos + 1);
        env.insert(name, value);
    }

    QJsonObject settings;
    settings.insert("env", env);
    settings.insert("experimentalWorkspaceModule", true);
    settings.insert("experimentalUseInvalidMetadata", true);
    settings.insert("allowModfileModifications", true);
    settings.insert("allowImplicitNetworkAccess", true);
    settings.insert("staticcheck", true);
    settings.insert("semanticTokens", true);
    settings.insert("linksInHover", false);
    settings.insert("completeUnimported", true);
    settings.insert("ui.semanticTokens", true);
    QJsonObject analyses;
    analyses.insert("nilness", true);
    analyses.insert("shadow", true);
    settings.insert("analyses", analyses);
    params->setInitializationOptions(new QJsonValue(settings));
    clientCapabilites->setExperimental(new QJsonValue(settings));
    CodeLensClientCapabilities *codeLens = new CodeLensClientCapabilities;
    codeLens->setDynamicRegistration(new bool(true));
    documentCapabilities->setCodeLens(codeLens);

    CodeActionClientCapabilities *codeAction = new CodeActionClientCapabilities;
    CodeActionClientCapabilitiesLiteralSupport *codeActionLiteral = new CodeActionClientCapabilitiesLiteralSupport;
    CodeActionClientCapabilitiesKind *codeActionLiteralKind = new CodeActionClientCapabilitiesKind;
    QList<CodeActionKind *> *actions = new QList<CodeActionKind *>();
    *actions << new CodeActionKind(SourceOrganizeImports);
    *actions << new CodeActionKind(RefactorRewrite);
    *actions << new CodeActionKind(RefactorExtract);
    codeActionLiteralKind->setValueSet(actions);
    codeActionLiteral->setCodeActionKind(codeActionLiteralKind);
    codeAction->setCodeActionLiteralSupport(codeActionLiteral);
    documentCapabilities->setCodeAction(codeAction);
    qDebug().noquote() << params->toJson();
    sendCommand(MethodInitialize, params, DECODE_CALLBACK(&GoPlsServer::decodeInitialize), "", true);
}

void GoPlsServer::prepareRenameSymbol(const QString &path, unsigned int line, unsigned int column)
{
    QSharedPointer<PrepareRenameParams> params(new PrepareRenameParams);
    params->setTextDocument(documentIdentifier(path));
    params->setPosition(position(line, column));
    sendCommand(MethodTextDocumentPrepareRename, params, DECODE_CALLBACK(&GoPlsServer::printResponse), path);
}

void GoPlsServer::askFindUsage(const QString &path, unsigned int line, unsigned int column)
{
    QSharedPointer<ReferenceParams> params(new ReferenceParams);
    ReferenceContext *ctx = new ReferenceContext;
    ctx->setIncludeDeclaration(new bool(true));
    params->setTextDocument(documentIdentifier(path));
    params->setPosition(position(line, column));
    params->setContext(ctx);
    sendCommand(MethodTextDocumentReferences, params, DECODE_CALLBACK(&GoPlsServer::decodeFindUsage), path);
}

void GoPlsServer::askAutocomplete(const QString &path, unsigned int line, unsigned int column)
{
    QSharedPointer<CompletionParams> params(new CompletionParams());
    params->setTextDocument(documentIdentifier(path));
    params->setPosition(position(line, column));
    CompletionContext *context = new CompletionContext();
    context->setTriggerCharacter(new QString("."));
    context->setTriggerKind(new CompletionTriggerKind(CompletionTriggerKindTriggerCharacter));
    params->setContext(context);
    sendCommand(MethodTextDocumentCompletion, params, DECODE_CALLBACK(&GoPlsServer::decodeDocumentCompletion), path);
}

void GoPlsServer::askSignatureHelp(const QString &path, unsigned int line, unsigned int column, const QString &trigger)
{
    QSharedPointer<SignatureHelpParams> params(new SignatureHelpParams);
    SignatureHelpContext *context = new SignatureHelpContext;
    context->setTriggerKind(new SignatureHelpTriggerKind(SignatureHelpTriggerKindTriggerCharacter));
    context->setTriggerCharacter(new QString(trigger));
    params->setContext(context);
    params->setTextDocument(documentIdentifier(path));
    params->setPosition(position(line, column));
    sendCommand(MethodTextDocumentSignatureHelp, params, DECODE_CALLBACK(&GoPlsServer::decodeSignatureHelp), path);
}

void GoPlsServer::updateWorkspaceFolders(const QStringList &add, const QStringList &remove)
{
    m_init = false;
    QSharedPointer<DidChangeWorkspaceFoldersParams> params(new DidChangeWorkspaceFoldersParams);
    WorkspaceFoldersChangeEvent *event = new WorkspaceFoldersChangeEvent();
    QList<WorkspaceFolder *> *adds = new QList<WorkspaceFolder *>();
    for (auto &folder : add) {
        WorkspaceFolder *wFolder = new WorkspaceFolder();
        wFolder->setName(new QString(QFileInfo(folder).baseName()));
        wFolder->setUri(new QString("file://" + folder));
        adds->append(wFolder);
    }
    event->setAdded(adds);

    QList<WorkspaceFolder *> *removes = new QList<WorkspaceFolder *>();
    for (auto &folder : remove) {
        WorkspaceFolder *wFolder = new WorkspaceFolder();
        wFolder->setName(new QString(QFileInfo(folder).baseName()));
        wFolder->setUri(new QString("file://" + folder));
        removes->append(wFolder);
    }
    event->setRemoved(removes);
    params->setEvent(event);
    sendCommand(MethodWorkspaceDidChangeWorkspaceFolders, params, DECODE_CALLBACK(&GoPlsServer::decodeAddWorkspaceFolder), "", true);
}

void GoPlsServer::askDefinitions(const QString &file, bool hover, unsigned int line, unsigned int column)
{
    QSharedPointer<DefinitionParams> params(new DefinitionParams());
    params->setTextDocument(documentIdentifier(file));
    params->setPosition(position(line, column));
    if (hover) {
        sendCommand(MethodTextDocumentDefinition, params, DECODE_CALLBACK(&GoPlsServer::decodeDocumentDefinitionHover), file);
    } else {
        sendCommand(MethodTextDocumentDefinition, params, DECODE_CALLBACK(&GoPlsServer::decodeDocumentDefinitionGoTo), file);
    }
}

void GoPlsServer::formatDocument(const QString &file)
{
    QSharedPointer<DocumentFormattingParams> params(new DocumentFormattingParams());
    params->setTextDocument(documentIdentifier(file));
    sendCommand(MethodTextDocumentFormatting, params, DECODE_CALLBACK(&GoPlsServer::decodeDocumentFormatting), file);
}

void GoPlsServer::organizeImports(const QString &file)
{
    QSharedPointer<CodeActionParams> params(new CodeActionParams());
    params->setTextDocument(documentIdentifier(file));
    CodeActionContext *context = new CodeActionContext;
    QList<CodeActionKind *> *actions = new QList<CodeActionKind *>();
    *actions << new CodeActionKind(SourceOrganizeImports);
    *actions << new CodeActionKind(RefactorRewrite);
    context->setOnly(actions);
    params->setContext(context);
    sendCommand(MethodTextDocumentCodeAction, params, DECODE_CALLBACK(&GoPlsServer::decodeOrganizeImport), file);
}

void GoPlsServer::documentHighlight(const QString &file, int startLine, int startColumn, int endLine, int endColumn)
{
    QSharedPointer<DocumentHighlightParams> params(new DocumentHighlightParams);
    params->setTextDocument(documentIdentifier(file));
    sendCommand(MethodTextDocumentDocumentHighlight, params, nullptr, file);
}

void GoPlsServer::fileOpened(const QString &file, const QString &content)
{
    m_openedFiles.insert(file, true);
    QSharedPointer<DidOpenTextDocumentParams> params(new DidOpenTextDocumentParams());
    TextDocumentItem *item = new TextDocumentItem();
    item->setUri(new DocumentURI("file://" + file));
    item->setLanguageId(new LanguageIdentifier(GoLanguage));
    item->setText(new QString(content));
    params->setTextDocument(item);
    sendCommand(MethodTextDocumentDidOpen, params, DECODE_CALLBACK(&GoPlsServer::decodeDidOpened), file);
}

void GoPlsServer::fileClosed(const QString &file)
{
    m_openedFiles.remove(file);
    m_filesVersions.remove(file);

    QSharedPointer<DidCloseTextDocumentParams> params(new DidCloseTextDocumentParams());
    params->setTextDocument(documentIdentifier(file));
    sendCommand(MethodTextDocumentDidClose, params, DECODE_CALLBACK(&GoPlsServer::decodeDidClosed), file);
}

void GoPlsServer::hover(const QString &filename, int line, int column)
{
    QSharedPointer<HoverParams> params(new HoverParams);
    params->setTextDocument(documentIdentifier(filename));
    params->setPosition(position(line, column));
    sendCommand(MethodTextDocumentHover, params, DECODE_CALLBACK(&GoPlsServer::decodeHover), filename);
}

void GoPlsServer::documentSymbols(const QString &filename)
{
    QSharedPointer<DocumentSymbolParams> params(new DocumentSymbolParams);
    params->setTextDocument(documentIdentifier(filename));
    sendCommand(MethodTextDocumentDocumentSymbol, params, DECODE_CALLBACK(&GoPlsServer::decodeDocumentSymbols), filename);
}

void GoPlsServer::enableStaticcheck(bool v)
{

    QSharedPointer<DidChangeConfigurationParams> params(new DidChangeConfigurationParams());
    QJsonObject settings;
    settings.insert("staticcheck", v);
    params->setSettings(new QJsonValue(settings));
    sendCommand(MethodWorkspaceDidChangeConfiguration, params, DECODE_CALLBACK(&GoPlsServer::decodeCurrentEnvChanged), "");
}

void GoPlsServer::shutdown()
{
    sendCommand(MethodShutdown, nullptr, DECODE_CALLBACK(&GoPlsServer::decodeShutdown), "");
}

void GoPlsServer::exit()
{
    sendCommand(MethodExit, nullptr, DECODE_CALLBACK(&GoPlsServer::decodeExit), "");
}

void GoPlsServer::decodeInitialize(const CommandData &data, const QJsonObject &resp)
{
    printResponse(data, resp);
    QSharedPointer<InitializedParams> params(new InitializedParams());
    sendCommand(MethodInitialized, params, DECODE_CALLBACK(&GoPlsServer::decodeInitialized), "", true);
}

void GoPlsServer::decodeInitialized(const CommandData &data, const QJsonObject &resp)
{
    printResponse(data, resp);
    m_init = true;
    if (!m_waitingCommands.isEmpty()) {
        executeCommand(m_waitingCommands.takeFirst());
    }
}

QList<DefinitionResult> GoPlsServer::decodeDocumentDefinition(const CommandData &data, const QJsonObject &jsonObject)
{
    QJsonArray locations = jsonObject.value("result").toArray();
    QList<DefinitionResult> list;
    for (const QJsonValue &value : locations) {
        Location *location = new Location();
        location->fromJson(value.toObject());
        DefinitionResult result;
        result.path = location->getUri()->replace("file://", "");
        const auto pos = location->getRange()->getEnd();
        result.line = *pos->getLine();
        result.column = *pos->getCharacter();
        list << result;
    }
    return list;
}

void GoPlsServer::decodeDocumentDefinitionHover(const CommandData &data, const QJsonObject &response)
{
    auto list = decodeDocumentDefinition(data, response);
    emit hoverDefinitionResult(data.filepath, list);
}

void GoPlsServer::decodeDocumentDefinitionGoTo(const CommandData &data, const QJsonObject &response)
{
    auto list = decodeDocumentDefinition(data, response);
    emit definitionsResult(data.filepath, list);
}

void GoPlsServer::decodeDocumentCompletion(const CommandData &data, const QJsonObject &jsonObject)
{
    CompletionList list;
    list.fromJson(jsonObject.value("result").toObject());
    QList<AutoCompletionResult> completions;
    if (list.getItems()) {
        for (auto it = list.getItems()->cbegin(); it != list.getItems()->cend(); ++it) {
            AutoCompletionResult result;
            auto label = (*it)->getLabel();
            auto detail = (*it)->getDetail();
            auto kind = (*it)->getKind();
            if (label) {
                result.label = *label;
            }
            if (detail) {
                result.detail = *detail;
            }
            if (kind) {
                switch (int(*kind)) {
                    case CompletionItemKindValue:
                    case CompletionItemKindConstant:
                        result.type = "const";
                        break;
                    case CompletionItemKindFunction:
                        result.type = "func";
                        break;
                    case CompletionItemKindInterface:
                    case CompletionItemKindStruct:
                        result.type = "struct";
                        break;
                    case CompletionItemKindVariable:
                        result.type = "var";
                        break;
                    case CompletionItemKindModule:
                        result.type = "package";
                        break;
                    default:
                        result.type = "type";
                        break;
                }
            }
            completions << result;
        }
    }
    emit autocompleteResult(data.filepath, completions);
}

QList<TextEditResult> convertTextEditResult(QJsonArray array)
{
    QList<TextEditResult> list;
    for (const auto it : array) {
        TextEdit edit;
        edit.fromJson(it.toObject());
        TextEditResult result;
        result.text = *edit.getNewText();
        result.startLine = *edit.getRange()->getStart()->getLine();
        result.startColumn = *edit.getRange()->getStart()->getCharacter();
        result.endLine = *edit.getRange()->getEnd()->getLine();
        result.endColumn = *edit.getRange()->getEnd()->getCharacter();
        list << result;
    }
    return list;
}

void GoPlsServer::decodeDocumentFormatting(const CommandData &data, const QJsonObject &jsonObject)
{
    const QJsonArray array = jsonObject.value("result").toArray();
    emit formattingResults(data.filepath, convertTextEditResult(array));
}

void GoPlsServer::decodeOrganizeImport(const CommandData &data, const QJsonObject &jsonObject)
{
    QJsonArray results = jsonObject.value("result").toArray();
    for (const auto &result : results) {
        QJsonArray edits = result.toObject().value("edit").toObject().value("documentChanges").toArray();
        for (const auto &edit : edits) {
            const QJsonObject obj = edit.toObject();
            const QString filename = obj.value("textDocument").toObject().value("uri").toString().mid(7);
            const QList<TextEditResult> modifications(convertTextEditResult(obj.value("edits").toArray()));
            emit updateFile(filename, modifications);
        }
    }
}

void GoPlsServer::decodeHover(const CommandData &data, const QJsonObject &response)
{
    if (response.value("result").isNull()) {
        return;
    }
    auto contents = response.value("result");
    if (contents.isArray()) {

    } else {
        Hover hover;
        hover.fromJson(contents.toObject());
        auto range = hover.getRange();
        HoverResult result;
        result.startLine = *range->getStart()->getLine();
        result.startColumn = *range->getStart()->getCharacter();
        result.endLine = *range->getEnd()->getLine();
        result.endColumn = *range->getEnd()->getCharacter();
        result.info = *hover.getContents()->getValue();
        emit hoverResult(data.filepath, {result});
    }
}

void GoPlsServer::decodeDiagnostics(const CommandData &data, const QJsonObject &response)
{
    PublishDiagnosticsParams result;
    result.fromJson(response.value("params").toObject());
    QList<DiagnosticResult> list;
    auto diags = *result.getDiagnostics();
    for (auto it : diags) {
        DiagnosticResult diag;
        diag.message = *it->getMessage();
        if (it->getSource()) {
            diag.code = *it->getSource();
        }
        diag.line = *it->getRange()->getStart()->getLine() + 1;

        /*
        DiagnosticSeverityError DiagnosticSeverity = 1

        // DiagnosticSeverityWarning reports a warning.
        DiagnosticSeverityWarning DiagnosticSeverity = 2

        // DiagnosticSeverityInformation reports an information.
        DiagnosticSeverityInformation DiagnosticSeverity = 3

        // DiagnosticSeverityHint reports a hint.
        DiagnosticSeverityHint DiagnosticSeverity = 4
         */
        switch (int(*it->getSeverity())) {
            case 1:
                diag.level = "error";
                break;
            case 2:
                diag.level = "warning";
                break;
            case 3:
                diag.level = "info";
                break;
            case 4:
                diag.level = "hint";
                break;
        }
        list << diag;
    }
    emit diagnosticsInfo(result.getUri()->mid(7), list);
}

void GoPlsServer::decodeDocumentSymbols(const CommandData &data, const QJsonObject &response)
{
    const auto symbols = response.value("result").toArray();
    QHash<QString, QList<LiteApi::Symbol>> result;
    for (auto it : symbols) {
        SymbolInformation symbol;
        symbol.fromJson(it.toObject());
        LiteApi::Symbol res;
        res.startLine = *symbol.getLocation()->getRange()->getStart()->getLine();
        res.endLine = *symbol.getLocation()->getRange()->getEnd()->getLine();
        res.name = *symbol.getName();

        const QString filename = symbol.getLocation()->getUri()->mid(7);
        auto list = result[filename];
        list << res;
        result[filename] = list;
    }
    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        emit documentSymbolsResult(it.key(), it.value());
    }
}

void GoPlsServer::decodeDidChanged(const CommandData &data, const QJsonObject &response)
{
}

void GoPlsServer::decodeDidSaved(const CommandData &data, const QJsonObject &response)
{
}

void GoPlsServer::decodeShutdown(const CommandData &data, const QJsonObject &response)
{
    exit();
}

void GoPlsServer::decodeExit(const CommandData &data, const QJsonObject &response)
{

}

void GoPlsServer::decodeAddWorkspaceFolder(const CommandData &data, const QJsonObject &response)
{
    m_init = true;
    if (!m_waitingCommands.isEmpty()) {
        executeCommand(m_waitingCommands.takeFirst());
    }
}

void GoPlsServer::decodeCurrentEnvChanged(const CommandData &data, const QJsonObject &response)
{
}

void GoPlsServer::decodeSignatureHelp(const CommandData &data, const QJsonObject &response)
{
}

void GoPlsServer::decodeFindUsage(const CommandData &data, const QJsonObject &response)
{
    QList<UsageResult> result;
    for (auto it : response.value("result").toArray()) {
        Location item;
        item.fromJson(it.toObject());
        UsageResult res;
        res.filepath = item.getUri()->mid(7);
        res.startLine = *item.getRange()->getStart()->getLine();
        res.startColumn = *item.getRange()->getStart()->getCharacter();
        res.endLine = *item.getRange()->getEnd()->getLine();
        res.endColumn = *item.getRange()->getEnd()->getCharacter();
        result << res;
    }
    emit findUsageResult(data.filepath, result);
}

void GoPlsServer::decodeSemanticTokens(const CommandData &data, const QJsonObject &response)
{
    printResponse(data, response);
    emit semanticTokensResult(data.filepath, response.value("result").toObject().value("data").toArray().toVariantList());
}

void GoPlsServer::printResponse(const CommandData &data, const QJsonObject &response)
{
    qDebug().noquote() << response;
}

void GoPlsServer::decodeDidOpened(const CommandData &data, const QJsonObject &response)
{
    QSharedPointer<SemanticTokensParams> params(new SemanticTokensParams);
    params->setTextDocument(documentIdentifier(data.filepath));
    sendCommand(MethodSemanticTokensFull, params, DECODE_CALLBACK(&GoPlsServer::decodeSemanticTokens), data.filepath);
}

void GoPlsServer::decodeDidClosed(const CommandData &data, const QJsonObject &response)
{
    documentSymbols(data.filepath);
}

void GoPlsServer::fileSaved(const QString &file, const QString &content)
{
    QSharedPointer<DidSaveTextDocumentParams> params(new DidSaveTextDocumentParams());
    params->setTextDocument(documentIdentifier(file));
    params->setText(new QString(content));
    sendCommand(MethodTextDocumentDidSave, params, DECODE_CALLBACK(&GoPlsServer::decodeDidSaved), file);
}

void GoPlsServer::fileChanged(const QString &file, int startLine, int startPos, int endLine, int endPos, const QString &content)
{
    QSharedPointer<DidChangeTextDocumentParams> params(new DidChangeTextDocumentParams());
    VersionedTextDocumentIdentifier *item = new VersionedTextDocumentIdentifier();
    item->setUri(new DocumentURI("file://" + file));
    const int version = m_filesVersions.value(file);
    m_filesVersions[file] = version + 1;
    item->setVersion(new int(version));
    params->setTextDocument(item);
    QList<TextDocumentContentChangeEvent *> *list = new QList<TextDocumentContentChangeEvent *>();
    TextDocumentContentChangeEvent *text = new TextDocumentContentChangeEvent();
    text->setRange(range(position(startLine, startPos), position(endLine, endPos)));
    text->setText(new QString(content));
    list->append(text);
    params->setContentChanges(list);
    sendCommand(MethodTextDocumentDidChange, params, DECODE_CALLBACK(&GoPlsServer::decodeDidChanged), file);
}

TextDocumentIdentifier *GoPlsServer::documentIdentifier(const QString &path) const
{
    TextDocumentIdentifier *identifier = new TextDocumentIdentifier();
    identifier->setUri(new DocumentURI("file://" + path));
    return identifier;
}

GoPlsTypes::Position *GoPlsServer::position(unsigned int line, unsigned int column) const
{
    Position *pos = new Position;
    pos->setLine(new unsigned int(line));
    pos->setCharacter(new unsigned int(column));
    return pos;
}

GoPlsTypes::Range *GoPlsServer::range(GoPlsTypes::Position *start, GoPlsTypes::Position *end) const
{
    Range *range = new Range;
    range->setStart(start);
    range->setEnd(end);
    return range;
}

void GoPlsServer::decodeResponse(const QByteArray &payload)
{
    if (payload.isEmpty()) {
        return;
    }
    QJsonObject jsonObject = QJsonDocument::fromJson(payload).object();
    const int commandID = jsonObject.value("id").toInt();

    auto data = m_idToData.take(commandID);
    auto callback = m_idToCallback.take(commandID);

    if (commandID == 0) {
        const QString method = jsonObject.value("method").toString();
        if (method == MethodWindowLogMessage || method == MethodWindowShowMessage) {
            LogMessageParams message;
            message.fromJson(jsonObject.value("params").toObject());
            if (message.getMessage()) {
                bool isError = message.getType() && *message.getType() == MessageTypeError;
                emit logMessage(*message.getMessage(), isError);
            }
        } else if (method == MethodTextDocumentPublishDiagnostics) {
            decodeDiagnostics(data, QJsonDocument::fromJson(payload).object());
        } else if (!payload.isEmpty()) {
            qDebug().noquote() << payload;
        }
    } else {
        const QString requested = data.command;
        if (callback) {
            callback(data, jsonObject);
        } else {
            qDebug().noquote() << requested << payload;
        }
    }
    if (!m_waitingCommands.empty() && m_init) {
        executeCommand(m_waitingCommands.takeFirst());
    }
}

void GoPlsServer::sendCommand(const QString &command, const QSharedPointer<GoPlsParams> &params, const DecodeFunc &responseFunc, const QString &filepath, bool force)
{
    GoPlsCommand cmd(command, params, responseFunc, filepath);
    if (!m_init && !force) {
        m_waitingCommands << cmd;
    } else {
        executeCommand(cmd);
    }
}

void GoPlsServer::executeCommand(const GoPlsCommand &cmd)
{
    auto cmdID = cmd.commandID();
    CommandData data;
    data.command = cmd.method();
    data.filepath = cmd.filepath();
    m_idToData.insert(cmdID, data);
    m_idToCallback.insert(cmdID, cmd.decodeFunc());
    QTimer::singleShot(10000, [this, cmdID] {
        m_idToData.take(cmdID);
        m_idToCallback.take(cmdID);
    });
    const QByteArray raw = cmd.toJson();
    const QString payload = QString("Content-Length: %1\r\n\r\n%2").arg(raw.length()).arg(QString::fromUtf8(raw));
    m_process->write(payload.toUtf8());
}

void GoPlsServer::onReadyRead()
{
    const QByteArray payload = m_process->readAll();
    if (payload.isEmpty()) {
        return;
    }
    int contentLength = 0;
    int offset = 0;
    if (payload.length() == 0) {
        return;
    }
    if (payload.indexOf("\r\n") < 0) {
        decodeResponse(payload);
        return;
    }
    int i = 0;
    while (true && i < 10) {
        const int next = payload.indexOf("\r\n", offset);
        if (next != offset) {
            QString header = payload.mid(offset, next - offset);
            if (header.startsWith("Content-Length:", Qt::CaseInsensitive)) {
                QString size = header.remove("Content-Length:", Qt::CaseInsensitive).trimmed();
                contentLength = size.toInt();
            }
            offset = next + 2;
        } else {
            const QByteArray body = payload.mid(offset + 2, contentLength);
            decodeResponse(body);
            offset += contentLength + 2;
        }
        if (offset >= payload.length()) {
            break;
        }
        i++;
    }
}

void GoPlsServer::onStarted()
{
}

void GoPlsServer::currentEnvChanged(LiteApi::IEnv *env)
{
    if (env) {
        auto list = env->environment().toStringList();
        QJsonObject obj;

        for (const QString &var : list) {
            auto pos = var.indexOf("=");
            const QString name = var.left(pos);
            const QString value = var.mid(pos + 1);
            obj.insert(name, value);
        }

        QSharedPointer<DidChangeConfigurationParams> params(new DidChangeConfigurationParams());
        QJsonObject settings;
        settings.insert("env", obj);
        params->setSettings(new QJsonValue(settings));
        sendCommand(MethodWorkspaceDidChangeConfiguration, params, DECODE_CALLBACK(&GoPlsServer::decodeCurrentEnvChanged), "");
    }
}
