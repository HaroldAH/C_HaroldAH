#include "stdafx.h"
#include "FileWatcherService.h"
#include <QFileSystemWatcher>
#include <QTimer>

FileWatcherService::FileWatcherService(QObject* parent)
    : QObject(parent)
{
    watcher = new QFileSystemWatcher(this);
    timer = new QTimer(this);
    timer->setInterval(500);
    timer->setSingleShot(false);
    
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcherService::onFileChanged);
    connect(timer, &QTimer::timeout, this, &FileWatcherService::onTimer);
}

void FileWatcherService::watch(const QString& path)
{
    try {
        // Desregistrar archivos previos
        QStringList watchedFiles = watcher->files();
        if (!watchedFiles.isEmpty()) {
            watcher->removePaths(watchedFiles);
            emit status("Dejando de vigilar archivos anteriores", false);
        }
        
        // Incrementar epoch (descarta recargas viejas)
        reloadEpoch++;
        
        // Actualizar ruta actual
        current = path;
        
        if (!path.isEmpty()) {
            if (watcher->addPath(path)) {
                emit status("Vigilando cambios en: " + path);
            } else {
                emit status("Advertencia: No se pudo vigilar", true);
            }
        }
    } catch (const std::exception& e) {
        emit status("Error al vigilar archivo: " + QString::fromStdString(e.what()), true);
    } catch (...) {
        emit status("Error desconocido al vigilar archivo", true);
    }
}

void FileWatcherService::unwatchAll()
{
    try {
        QStringList watchedFiles = watcher->files();
        if (!watchedFiles.isEmpty()) {
            watcher->removePaths(watchedFiles);
            emit status("Dejando de vigilar", false);
        }
        
        reloadEpoch++;
        current.clear();
        
    } catch (...) {
        emit status("Error al dejar de vigilar", true);
    }
}

void FileWatcherService::ignoreNextChange()
{
    ignoreOne = true;
}

void FileWatcherService::onFileChanged(const QString& path)
{
    try {
        if (ignoreOne) {
            ignoreOne = false;
            emit status("Cambio ignorado (guardado desde app)", false);
            return;
        }
        
        if (path != current) {
            return;
        }
        
        emit status("Detectado cambio: " + path, false);
        
        pendingEpoch = reloadEpoch;
        timer->start();
        
    } catch (const std::exception& e) {
        emit status("Error en onFileChanged: " + QString::fromStdString(e.what()), true);
    } catch (...) {
        emit status("Error desconocido en onFileChanged", true);
    }
}

void FileWatcherService::onTimer()
{
    try {
        // Descartar recarga si epoch cambió (archivo diferente cargado)
        if (pendingEpoch != reloadEpoch) {
            timer->stop();
            emit status("Recarga cancelada (archivo cambiado)", false);
            return;
        }
        
        if (current.isEmpty()) {
            timer->stop();
            return;
        }
        
        emit reloadRequested(current);
        timer->stop();
        
    } catch (const std::exception& e) {
        emit status("Error durante recarga: " + QString::fromStdString(e.what()), true);
        timer->stop();
    } catch (...) {
        emit status("Error desconocido durante recarga", true);
        timer->stop();
    }
}
