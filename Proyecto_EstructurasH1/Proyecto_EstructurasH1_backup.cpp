#include "stdafx.h"
#include "Proyecto_EstructurasH1.h"
#include "LinkedList.h"
#include <QApplication>
#include <QScreen>
#include <QTime>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

Proyecto_EstructurasH1::Proyecto_EstructurasH1(QWidget *parent)
    : QMainWindow(parent), processor(nullptr), isFileLoaded(false), isProcessed(false),
      inputTextEdit(nullptr), outputTextEdit(nullptr), statusTextEdit(nullptr),
      loadButton(nullptr), processButton(nullptr), validateButton(nullptr), 
      saveButton(nullptr), instructionLabel(nullptr)
{
    ui.setupUi(this);
    
    // Initialize processor
    processor = new NaturalLanguageProcessor();
    
    // Create basic UI
    createBasicUI();
    
    // Setup window properties
    setupWindowProperties();
    
    // Setup connections
    setupConnections();
    
    // Initialize interface
    resetInterface();
    
    // Welcome messages
    updateStatus("Bienvenido a C_HaroldA - Convertidor de Lenguaje Natural a C++");
    updateStatus("Desarrollado por: HAROLD AVIA HERNANDEZ");
    updateStatus("Universidad Nacional - Sede Regional Brunca - Campus PZ-Coto");
    updateStatus("Estructura de Datos II - Ciclo 2025");
    updateStatus("Profesores: Saray Castro - Hairol Romero");
    updateStatus("Cargue un archivo .txt para comenzar...");
}

Proyecto_EstructurasH1::~Proyecto_EstructurasH1()
{
    if (processor) {
        delete processor;
    }
}

void Proyecto_EstructurasH1::createBasicUI()
{
    // Create a central widget with basic controls
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    
    // Input area
    inputTextEdit = new QTextEdit();
    inputTextEdit->setPlaceholderText("Contenido del archivo se mostrará aquí...");
    layout->addWidget(new QLabel("Entrada (Lenguaje Natural):"));
    layout->addWidget(inputTextEdit);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    loadButton = new QPushButton("Cargar Archivo");
    processButton = new QPushButton("Procesar Conversion");
    validateButton = new QPushButton("Validar Sintaxis");
    saveButton = new QPushButton("Guardar Codigo");
    
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(saveButton);
    layout->addLayout(buttonLayout);
    
    // Output area
    outputTextEdit = new QTextEdit();
    outputTextEdit->setPlaceholderText("Código C++ generado aparecerá aquí...");
    layout->addWidget(new QLabel("Salida (Código C++):"));
    layout->addWidget(outputTextEdit);
    
    // Status area
    statusTextEdit = new QTextEdit();
    statusTextEdit->setMaximumHeight(150);
    statusTextEdit->setPlaceholderText("Estado del procesamiento...");
    layout->addWidget(new QLabel("Estado:"));
    layout->addWidget(statusTextEdit);
    
    // Instruction count
    instructionLabel = new QLabel("Instrucciones detectadas: 0");
    layout->addWidget(instructionLabel);
}

void Proyecto_EstructurasH1::setupWindowProperties()
{
    // Window title and properties
    setWindowTitle("C_HaroldA - Convertidor de Lenguaje Natural a C++");
    setMinimumSize(1200, 800);
    
    // Center window on screen
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenRect = screen->availableGeometry();
        QRect windowRect = geometry();
        move((screenRect.width() - windowRect.width()) / 2,
             (screenRect.height() - windowRect.height()) / 2);
    }
}

void Proyecto_EstructurasH1::setupConnections()
{
    // Button connections
    if (loadButton) connect(loadButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::loadFile);
    if (processButton) connect(processButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::processConversion);
    if (validateButton) connect(validateButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::validateSyntax);
    if (saveButton) connect(saveButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::saveCode);
}

void Proyecto_EstructurasH1::loadFile()
{
    try {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Cargar archivo de lenguaje natural - C_HaroldA",
            "",
            "Archivos de texto (*.txt);;Todos los archivos (*.*)"
        );

        if (!fileName.isEmpty()) {
            currentFilePath = fileName.toStdString();
            
            updateStatus("Cargando archivo: " + currentFilePath);
            QApplication::processEvents(); // Update UI
            
            if (processor->loadFile(currentFilePath)) {
                std::string content = processor->getInputContent();
                if (inputTextEdit) inputTextEdit->setPlainText(QString::fromStdString(content));
                isFileLoaded = true;
                isProcessed = false;
                
                updateStatus("Archivo cargado exitosamente!");
                updateStatus("Archivo: " + currentFilePath);
                updateInstructionCount();
                enableButtons();
                
                // Clear previous output
                if (outputTextEdit) outputTextEdit->clear();
                
                updateStatus("Listo para procesar. Haga clic en 'Procesar Conversion'");
            } else {
                updateStatus("Error al cargar el archivo", true);
                displayErrors();
                isFileLoaded = false;
                enableButtons();
            }
        }
    } catch (const std::exception& e) {
        updateStatus("Error inesperado al cargar archivo: " + std::string(e.what()), true);
        isFileLoaded = false;
        enableButtons();
    } catch (...) {
        updateStatus("Error desconocido al cargar archivo", true);
        isFileLoaded = false;
        enableButtons();
    }
}

void Proyecto_EstructurasH1::processConversion()
{
    try {
        if (!isFileLoaded) {
            updateStatus("Error: Debe cargar un archivo primero", true);
            return;
        }
        
        if (!processor) {
            updateStatus("Error: Procesador no inicializado", true);
            return;
        }
        
        updateStatus("Iniciando procesamiento de lenguaje natural...");
        updateStatus("Analizando instrucciones del archivo...");
        QApplication::processEvents(); // Update UI
        
        processor->processInstructions();
        
        std::string generatedCode = processor->getGeneratedCode();
        if (outputTextEdit) outputTextEdit->setPlainText(QString::fromStdString(generatedCode));
        
        isProcessed = true;
        enableButtons();
        
        // Display any errors or warnings
        LinkedList<std::string> errors = processor->getErrors();
        if (errors.getSize() > 0) {
            updateStatus("Conversion completada con " + std::to_string(errors.getSize()) + " advertencias", true);
            displayErrors();
        } else {
            updateStatus("Conversion completada exitosamente!");
            updateStatus("Codigo C++ generado y listo para validar/guardar");
        }
        
        updateStatus("Se procesaron " + std::to_string(processor->getInstructionCount()) + " instrucciones");
        
    } catch (const std::exception& e) {
        updateStatus("Error durante la conversion: " + std::string(e.what()), true);
        isProcessed = false;
        enableButtons();
    } catch (...) {
        updateStatus("Error desconocido durante la conversion", true);
        isProcessed = false;
        enableButtons();
    }
}

void Proyecto_EstructurasH1::validateSyntax()
{
    if (!isProcessed) {
        updateStatus("Error: Debe procesar la conversion primero", true);
        return;
    }
    
    updateStatus("Iniciando validacion de sintaxis C++...");
    QApplication::processEvents();
    
    bool isValid = processor->validateSyntax();
    
    if (isValid) {
        updateStatus("Validacion exitosa: El codigo C++ generado es sintacticamente correcto");
        updateStatus("El codigo esta listo para ser compilado en un IDE de C++");
    } else {
        updateStatus("Validacion fallida: Se encontraron errores de sintaxis", true);
        updateStatus("Revise los mensajes de error a continuacion:", true);
        displayErrors();
    }
}

void Proyecto_EstructurasH1::saveCode()
{
    if (!isProcessed) {
        updateStatus("Error: Debe procesar la conversion primero", true);
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Guardar codigo C++ generado - C_HaroldA",
        "codigo_generado_HaroldA.cpp",
        "Archivos C++ (*.cpp);;Archivos de texto (*.txt);;Todos los archivos (*.*)"
    );

    if (!fileName.isEmpty()) {
        std::string filePath = fileName.toStdString();
        
        updateStatus("Guardando codigo C++ en: " + filePath);
        QApplication::processEvents();
        
        if (processor->saveToFile(filePath)) {
            updateStatus("Codigo C++ guardado exitosamente!");
            updateStatus("Archivo: " + filePath);
            updateStatus("Puede abrir este archivo en Visual Studio o cualquier IDE de C++");
        } else {
            updateStatus("Error al guardar el archivo", true);
            displayErrors();
        }
    }
}

void Proyecto_EstructurasH1::showAbout()
{
    QString aboutText = 
        "<h2>C_HaroldA - Convertidor de Lenguaje Natural a C++</h2>"
        "<hr>"
        "<p><b>Version:</b> 1.0</p>"
        "<p><b>Desarrollado por:</b> HAROLD AVIA HERNANDEZ</p>"
        "<p><b>Universidad:</b> Universidad Nacional - Sede Regional Brunca</p>"
        "<p><b>Campus:</b> Perez Zeledon - Coto</p>"
        "<p><b>Curso:</b> Estructura de Datos II - Ciclo 2025</p>"
        "<p><b>Profesores:</b> Saray Castro - Hairol Romero</p>";
    
    QMessageBox::about(this, "Acerca de C_HaroldA", aboutText);
}

void Proyecto_EstructurasH1::showInstructions()
{
    QString instructionsText =
        "<h2>Manual de Usuario - C_HaroldA</h2>"
        "<h3>Pasos para usar el convertidor:</h3>"
        "<ol>"
        "<li><b>Cargar Archivo:</b> Haga clic en 'Cargar Archivo' y seleccione un archivo .txt con instrucciones en lenguaje natural</li>"
        "<li><b>Procesar:</b> Haga clic en 'Procesar Conversion' para convertir las instrucciones a codigo C++</li>"
        "<li><b>Validar:</b> Use 'Validar Sintaxis' para verificar que el codigo generado sea correcto</li>"
        "<li><b>Guardar:</b> Guarde el codigo C++ generado usando 'Guardar Codigo C++'</li>"
        "</ol>";
    
    QMessageBox::information(this, "Manual de Usuario - C_HaroldA", instructionsText);
}

void Proyecto_EstructurasH1::updateStatus(const std::string& message, bool isError)
{
    QString timestamp = QTime::currentTime().toString("[hh:mm:ss] ");
    QString fullMessage = timestamp + QString::fromStdString(message);
    
    if (isError) {
        fullMessage = "<span style='color: #DC143C; font-weight: bold;'>X " + fullMessage + "</span>";
    } else {
        fullMessage = "<span style='color: #2E8B57;'>OK " + fullMessage + "</span>";
    }
    
    if (statusTextEdit) {
        statusTextEdit->append(fullMessage);
        
        // Auto-scroll to bottom
        QTextCursor cursor = statusTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        statusTextEdit->setTextCursor(cursor);
    }
    
    // Update status bar if it exists
    if (statusBar()) {
        statusBar()->showMessage(QString::fromStdString(message), 5000);
    }
}

void Proyecto_EstructurasH1::updateInstructionCount()
{
    if (processor && instructionLabel) {
        int count = processor->getInstructionCount();
        instructionLabel->setText("Instrucciones detectadas: " + QString::number(count));
    }
}

void Proyecto_EstructurasH1::enableButtons()
{
    if (processButton) processButton->setEnabled(isFileLoaded);
    if (validateButton) validateButton->setEnabled(isProcessed);
    if (saveButton) saveButton->setEnabled(isProcessed);
    
    // Update button text based on state
    if (processButton) {
        if (isFileLoaded && !isProcessed) {
            processButton->setText("Procesar Conversión");
        } else if (isProcessed) {
            processButton->setText("Reprocesar");
        }
    }
}

void Proyecto_EstructurasH1::resetInterface()
{
    isFileLoaded = false;
    isProcessed = false;
    enableButtons();
}

void Proyecto_EstructurasH1::displayErrors()
{
    if (processor) {
        LinkedList<std::string> errors = processor->getErrors();
        
        if (errors.getSize() > 0) {
            updateStatus("--- ERRORES Y ADVERTENCIAS ---", true);
            for (int i = 0; i < errors.getSize(); i++) {
                updateStatus("[" + std::to_string(i+1) + "] " + errors.get(i), true);
            }
            updateStatus("--- FIN DE ERRORES ---", true);
        }
    }
}

