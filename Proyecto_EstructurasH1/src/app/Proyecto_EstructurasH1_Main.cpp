#include "stdafx.h"
#include "../../Proyecto_EstructurasH1.h"
#include "EditorController.h"
#include "FileWatcherService.h"
#include "ProcessingController.h"
#include "../ui/DSLAssistantPanel.h"
#include <QFileDialog>
#include <QMessageBox>

// Stubs temporales para compatibilidad con código legacy
void Proyecto_EstructurasH1::loadFile()
{
    onCargarClicked();
}

void Proyecto_EstructurasH1::processConversion()
{
    onProcesarClicked();
}

void Proyecto_EstructurasH1::validateSyntax()
{
    onValidarClicked();
}

void Proyecto_EstructurasH1::saveCode()
{
    onGuardarClicked();
}

void Proyecto_EstructurasH1::showAbout()
{
    QString aboutText = 
        "C_HaroldA - Convertidor DSL a C++\n"
        "Version: 2.0\n"
        "Harold Avia Hernandez\n"
        "Universidad Nacional - Brunca\n"
        "Estructuras de Datos II - 2025";
    
    QMessageBox msgBox;
    msgBox.setWindowTitle("Acerca de C_HaroldA");
    msgBox.setText(aboutText);
    msgBox.setStyleSheet("QMessageBox { background-color: #111317; color: #fde68a; }");
    msgBox.exec();
}

void Proyecto_EstructurasH1::showInstructions()
{
    showHelpDialog();
}

// Implementaciones de slots de botones que delegan a controladores
void Proyecto_EstructurasH1::onEntradaTextChanged()
{
    if (inputTextEdit) {
        hasInput = !inputTextEdit->toPlainText().trimmed().isEmpty();
        updateButtonStates();
    }
}

void Proyecto_EstructurasH1::onNuevoClicked()
{
    if (editor) {
        editor->clearBothEditors();
    }
    if (watcher) {
        watcher->unwatchAll();
    }
    resetInterface();
    updateStatus("Nuevo archivo creado");
}

void Proyecto_EstructurasH1::onCargarClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Abrir Archivo de Lenguaje Natural",
        QString(),
        "Archivos de Texto (*.txt);;Todos los archivos (*.*)"
    );
    
    if (!path.isEmpty() && editor) {
        if (editor->loadFromFile(path)) {
            if (watcher) {
                watcher->watch(path);
            }
            hasInput = true;
            hasGenerated = false;
            isValidated = false;
            updateButtonStates();
        }
    }
}

void Proyecto_EstructurasH1::onProcesarClicked()
{
    if (processorCtl) {
        processorCtl->process();
    }
}

void Proyecto_EstructurasH1::onValidarClicked()
{
    if (processorCtl) {
        processorCtl->validate();
    }
}

void Proyecto_EstructurasH1::onGuardarClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "Guardar Codigo C++",
        QString(),
        "Archivos C++ (*.cpp);;Todos los archivos (*.*)"
    );
    
    if (!path.isEmpty() && editor) {
        bool savingWatchedFile = false;
        if (editor->saveToFile(path, &savingWatchedFile)) {
            if (savingWatchedFile && watcher) {
                watcher->ignoreNextChange();
            }
        }
    }
}

void Proyecto_EstructurasH1::onAyudaClicked()
{
    showHelpDialog();
}

void Proyecto_EstructurasH1::onReloadRequested(const QString& path)
{
    if (editor && !path.isEmpty()) {
        updateStatus("Recargando archivo modificado...");
        editor->loadFromFile(path);
        hasInput = true;
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    }
}

// Métodos helper
void Proyecto_EstructurasH1::resetInterface()
{
    hasInput = false;
    hasGenerated = false;
    isValidated = false;
    updateButtonStates();
}

void Proyecto_EstructurasH1::updateButtonStates()
{
    if (processButton) processButton->setEnabled(hasInput);
    if (validateButton) validateButton->setEnabled(hasGenerated);
    if (saveButton) saveButton->setEnabled(isValidated);
}

void Proyecto_EstructurasH1::enableButtons()
{
    if (processButton) processButton->setEnabled(true);
    if (validateButton) validateButton->setEnabled(true);
    if (saveButton) saveButton->setEnabled(true);
}
