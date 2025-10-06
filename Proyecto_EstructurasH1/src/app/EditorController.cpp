#include "stdafx.h"
#include "EditorController.h"
#include <QTextEdit>
#include <QFile>
#include <QFileDialog>
#include <QSaveFile>

EditorController::EditorController(QTextEdit* entrada, QTextEdit* salida, QObject* parent)
    : QObject(parent), input(entrada), output(salida)
{
}

bool EditorController::loadFromFile(const QString& path)
{
    try {
        if (path.isEmpty()) {
            emit status("Error: Ruta vacia", true);
            return false;
        }
        
        currentFile = path;
        emit status("Cargando archivo: " + path);
        
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit status("Error al abrir: " + path, true);
            return false;
        }
        
        QByteArray bytes = file.readAll();
        file.close();
        
        // UTF-8 primero, fallback a Local8Bit
        QString content = QString::fromUtf8(bytes);
        if (content.isNull() || content.isEmpty()) {
            content = QString::fromLocal8Bit(bytes);
            emit status("Archivo leido con codificacion local", false);
        }
        
        if (input) {
            QSignalBlocker blockEntrada(input);
            input->setPlainText(content);
        }
        
        if (output) {
            QSignalBlocker blockSalida(output);
            output->clear();
        }
        
        emit status("Archivo cargado exitosamente!");
        emit status("Listo para procesar");
        return true;
        
    } catch (const std::exception& e) {
        emit status("Excepcion al cargar: " + QString::fromStdString(e.what()), true);
        return false;
    } catch (...) {
        emit status("Excepcion desconocida al cargar", true);
        return false;
    }
}

bool EditorController::saveToFile(const QString& path, bool* savedWatchedFile)
{
    try {
        if (path.isEmpty()) {
            emit status("Error: Ruta vacia", true);
            return false;
        }
        
        if (!output) {
            emit status("Error: Editor de salida no disponible", true);
            return false;
        }
        
        QString generatedCode = output->toPlainText().trimmed();
        if (generatedCode.isEmpty()) {
            emit status("Error: No hay codigo", true);
            return false;
        }
        
        bool isSavingWatchedFile = (path == currentFile);
        if (savedWatchedFile) {
            *savedWatchedFile = isSavingWatchedFile;
        }
        
        emit status("Guardando en: " + path);
        
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit status("Error al abrir para escritura", true);
            return false;
        }
        
        QTextStream stream(&file);
        stream << generatedCode;
        
        if (!file.commit()) {
            emit status("Error al confirmar escritura", true);
            return false;
        }
        
        emit status("Guardado exitosamente!");
        emit status("Archivo: " + path);
        return true;
        
    } catch (const std::exception& e) {
        emit status("Error guardando: " + QString::fromStdString(e.what()), true);
        return false;
    } catch (...) {
        emit status("Error desconocido guardando", true);
        return false;
    }
}

QString EditorController::currentPath() const
{
    return currentFile;
}

void EditorController::setCurrentPath(const QString& p)
{
    currentFile = p;
}

void EditorController::clearBothEditors()
{
    if (input) {
        QSignalBlocker blockEntrada(input);
        input->clear();
        input->setPlainText("comenzar programa\n\nterminar programa\n");
    }
    
    if (output) {
        QSignalBlocker blockSalida(output);
        output->clear();
    }
    
    currentFile.clear();
}
