#ifndef LSPCONNECTIONHELPER_H
#define LSPCONNECTIONHELPER_H

#include <QJsonArray>
#include <QList>
#include <functional>

class ActionManager;
class ErrorListPanel;
class ReferencesPanel;
class TabManager;

namespace lsp
{

class LspServerManager;
struct Diagnostic;
struct Location;

void setupLspConnections(LspServerManager* manager, TabManager* tabManager, ErrorListPanel* errorPanel, ReferencesPanel* referencesPanel,
                         ActionManager* actionManager,
                         // line: 0-based. column: LSP UTF-16 code unit column.
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation);

} // namespace lsp

#endif // LSPCONNECTIONHELPER_H
