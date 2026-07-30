#ifndef LSPCONNECTIONHELPER_H
#define LSPCONNECTIONHELPER_H

#include <QJsonArray>
#include <QList>
#include <functional>

class ActionManager;
class ErrorListPanel;
class TabManager;

namespace lsp
{

class LspServerManager;
struct Diagnostic;
struct Location;

void setupLspConnections(LspServerManager* manager, TabManager* tabManager, ErrorListPanel* errorPanel, ActionManager* actionManager,
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation);

} // namespace lsp

#endif // LSPCONNECTIONHELPER_H
