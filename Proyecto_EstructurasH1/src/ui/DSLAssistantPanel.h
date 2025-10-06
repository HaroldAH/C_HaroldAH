#pragma once
#include <QObject>
#include <QString>

class QDockWidget;
class QTreeWidget;
class QTextEdit;
class QMainWindow;
class QTreeWidgetItem;

class DSLAssistantPanel : public QObject {
    Q_OBJECT
public:
    DSLAssistantPanel(QMainWindow* owner, QTextEdit* input, QObject* parent = nullptr);
    QDockWidget* dock() const;

private slots:
    void onSnippetDoubleClick(QTreeWidgetItem* item, int column);

private:
    void buildUi(QMainWindow* owner);
    void setupSnippets();
    void setupAutoCompleter();
    void insertSnippet(const QString& snippetId, bool useDialog);
    
    QDockWidget* dockWidget{};
    QTreeWidget* tree{};
    QTextEdit* inputEdit{};
};
