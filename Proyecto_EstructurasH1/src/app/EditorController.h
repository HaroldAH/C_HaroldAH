#pragma once
#include <QObject>
#include <QString>

class QTextEdit;

class EditorController : public QObject {
    Q_OBJECT
public:
    EditorController(QTextEdit* entrada, QTextEdit* salida, QObject* parent = nullptr);
    
    bool loadFromFile(const QString& path);
    bool saveToFile(const QString& path, bool* savedWatchedFile = nullptr);
    
    QString currentPath() const;
    void setCurrentPath(const QString& p);
    void clearBothEditors();

signals:
    void status(const QString& msg, bool error = false);

private:
    QTextEdit* input{};
    QTextEdit* output{};
    QString currentFile;
};
