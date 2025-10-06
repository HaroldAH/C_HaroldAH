#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QCompleter>
#include <QApplication>
#include <QScreen>
#include <QTime>
#include <atomic>
#include <string>
#include <memory>
#include "ui_Proyecto_EstructurasH1.h"
#include "NaturalLanguageProcessor.h"

// Forward declarations
class EditorController;
class FileWatcherService;
class ProcessingController;
class DSLAssistantPanel;
class QTreeWidgetItem;

class Proyecto_EstructurasH1 : public QMainWindow
{
    Q_OBJECT

public:
    Proyecto_EstructurasH1(QWidget *parent = nullptr);
    ~Proyecto_EstructurasH1();

private slots:
    // Slots originales (deprecated - mantener por compatibilidad)
    void loadFile();
    void processConversion();
    void validateSyntax();
    void saveCode();
    void showAbout();
    void showInstructions();
    
    // Slots para botones - ahora delegan a controladores
    void onEntradaTextChanged();
    void onNuevoClicked();
    void onCargarClicked();
    void onProcesarClicked();
    void onValidarClicked();
    void onGuardarClicked();
    void onAyudaClicked();
    
    // Slot para recarga de archivos
    void onReloadRequested(const QString& path);

private:
    Ui::Proyecto_EstructurasH1Class ui;
    
    // Core components
    NaturalLanguageProcessor* processor;
    std::string currentFilePath;
    
    // Nuevos controladores modulares
    EditorController* editor{};
    FileWatcherService* watcher{};
    ProcessingController* processorCtl{};
    DSLAssistantPanel* dslPanel{};
    
    // Button flow control flags
    bool hasInput = false;
    bool hasGenerated = false;
    bool isValidated = false;
    
    // UI Components (dynamically created)
    QTextEdit* inputTextEdit;
    QTextEdit* outputTextEdit;
    QTextEdit* statusTextEdit;
    QPushButton* loadButton;
    QPushButton* processButton;
    QPushButton* validateButton;
    QPushButton* saveButton;
    QPushButton* newButton;
    QLabel* instructionLabel;
    
    // Miembros para efectos Zenitsu
    QLabel* zenitsuAvatar;
    QLabel* zenitsuWatermark;
    
    // UI setup and management
    void createBasicUI();
    void setupConnections();
    void setupWindowProperties();
    void resetInterface();
    void enableButtons();
    void updateButtonStates();
    void newFile();
    
    // Status and messaging
    void updateStatus(const std::string& message, bool isError = false);
    void updateInstructionCount();
    void displayErrors();
    void displaySyntaxErrors();
    void showHelpDialog();
    
    // Helper para efectos Zenitsu
    QPixmap createCircularAvatar(const QString& imagePath, int size);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
};

