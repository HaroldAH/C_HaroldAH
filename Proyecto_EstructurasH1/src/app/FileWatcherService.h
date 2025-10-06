#pragma once
#include <QObject>
#include <QString>

class QFileSystemWatcher;
class QTimer;

class FileWatcherService : public QObject {
    Q_OBJECT
public:
    explicit FileWatcherService(QObject* parent = nullptr);
    
    void watch(const QString& path);
    void unwatchAll();
    void ignoreNextChange();

signals:
    void reloadRequested(const QString& path);
    void status(const QString& msg, bool error = false);

private slots:
    void onFileChanged(const QString& path);
    void onTimer();

private:
    QFileSystemWatcher* watcher{};
    QTimer* timer{};
    QString current;
    bool ignoreOne{false};
    int reloadEpoch{0};
    int pendingEpoch{0};
};
