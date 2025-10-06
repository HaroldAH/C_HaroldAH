#include "stdafx.h"
#include "../../Proyecto_EstructurasH1.h"
#include "EditorController.h"
#include "FileWatcherService.h"
#include "ProcessingController.h"
#include "../ui/DSLAssistantPanel.h"
#include <QApplication>
#include <QScreen>
#include <QTime>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainter>
#include <QPixmap>
#include <QDialog>
#include <QTextBrowser>
#include <QShortcut>
#include <QKeySequence>

// Constructor con integración de controladores
Proyecto_EstructurasH1::Proyecto_EstructurasH1(QWidget *parent)
    : QMainWindow(parent), processor(nullptr),
      inputTextEdit(nullptr), outputTextEdit(nullptr), statusTextEdit(nullptr),
      loadButton(nullptr), processButton(nullptr), validateButton(nullptr), 
      saveButton(nullptr), instructionLabel(nullptr),
      hasInput(false), hasGenerated(false), isValidated(false)
{
    ui.setupUi(this);
    
    // Crear procesador
    processor = new NaturalLanguageProcessor();
    
    // Crear UI básica
    createBasicUI();
    setupWindowProperties();
    
    // Crear controladores NUEVOS
    editor = new EditorController(inputTextEdit, outputTextEdit, this);
    watcher = new FileWatcherService(this);
    processorCtl = new ProcessingController(processor, inputTextEdit, outputTextEdit, this);
    dslPanel = new DSLAssistantPanel(this, inputTextEdit, this);
    
    // Conectar señales de controladores a updateStatus
    connect(editor, &EditorController::status, [this](const QString& msg, bool error) {
        updateStatus(msg.toStdString(), error);
    });
    
    connect(watcher, &FileWatcherService::status, [this](const QString& msg, bool error) {
        updateStatus(msg.toStdString(), error);
    });
    
    connect(watcher, &FileWatcherService::reloadRequested, 
            this, &Proyecto_EstructurasH1::onReloadRequested);
    
    connect(processorCtl, &ProcessingController::status, [this](const QString& msg, bool error) {
        updateStatus(msg.toStdString(), error);
    });
    
    connect(processorCtl, &ProcessingController::instructionCountChanged,
            [this](int count) {
                if (instructionLabel) {
                    instructionLabel->setText("Instrucciones detectadas: " + QString::number(count));
                }
            });
    
    connect(processorCtl, &ProcessingController::hasInputChanged,
            [this](bool state) { hasInput = state; updateButtonStates(); });
    
    connect(processorCtl, &ProcessingController::hasGeneratedChanged,
            [this](bool state) { hasGenerated = state; updateButtonStates(); });
    
    connect(processorCtl, &ProcessingController::isValidatedChanged,
            [this](bool state) { isValidated = state; updateButtonStates(); });
    
    // Conectar botones
    setupConnections();
    resetInterface();
    
    // Help shortcut
    QShortcut* helpShortcut = new QShortcut(QKeySequence::HelpContents, this);
    connect(helpShortcut, &QShortcut::activated, this, &Proyecto_EstructurasH1::onAyudaClicked);
    
    updateStatus("Bienvenido a C_HaroldA - Convertidor de Lenguaje Natural a C++");
    updateStatus("Desarrollado por: HAROLD AVIA HERNANDEZ");
    updateStatus("Universidad Nacional - Sede Regional Brunca - Campus PZ");
    updateStatus("Estructura de Datos II - Ciclo 2025");
    updateStatus("Profesores: Saray Castro - Hairol Romero");
    updateStatus("Escribe, inserta una plantilla o carga un .txt para comenzar...");
}

Proyecto_EstructurasH1::~Proyecto_EstructurasH1()
{
    if (processor) {
        delete processor;
    }
}

void Proyecto_EstructurasH1::createBasicUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    
    this->setStyleSheet(
        "QMainWindow { background-color: #0b0f14; }"
        "QWidget { background-color: #0b0f14; color: #fde68a; }"
        "QLabel { color: #fde68a; font-weight: bold; }"
    );
    
    inputTextEdit = new QTextEdit();
    inputTextEdit->setPlaceholderText("Contenido del archivo se mostrara aqui...");
    inputTextEdit->setStyleSheet(
        "QTextEdit { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; "
        "border-radius: 4px; padding: 8px; }"
        "QTextEdit::placeholder { color: #a16207; }"
    );
    
    layout->addWidget(new QLabel("Entrada (Lenguaje Natural):"));
    layout->addWidget(inputTextEdit);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    newButton = new QPushButton("Nuevo");
    loadButton = new QPushButton("Cargar Archivo");
    processButton = new QPushButton("Procesar Conversion");
    validateButton = new QPushButton("Validar Sintaxis");
    saveButton = new QPushButton("Guardar Codigo");
    QPushButton* helpButton = new QPushButton("Ayuda");
    
    QString buttonStyle = 
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
        "    color: #0b0f14; border: 1px solid #f59e0b; border-radius: 6px;"
        "    padding: 8px 16px; font-weight: bold; min-height: 20px;"
        "}"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fcd34d, stop:1 #f97316); }"
        "QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ea580c, stop:1 #fbbf24); }"
        "QPushButton:disabled { background-color: #111317; color: #a16207; border-color: #a16207; }";
    
    newButton->setStyleSheet(buttonStyle);
    loadButton->setStyleSheet(buttonStyle);
    processButton->setStyleSheet(buttonStyle);
    validateButton->setStyleSheet(buttonStyle);
    saveButton->setStyleSheet(buttonStyle);
    helpButton->setStyleSheet(buttonStyle);
    
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(helpButton);
    layout->addLayout(buttonLayout);
    
    connect(helpButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onAyudaClicked);
    
    outputTextEdit = new QTextEdit();
    outputTextEdit->setPlaceholderText("Codigo C++ generado aparecera aqui...");
    outputTextEdit->setStyleSheet(
        "QTextEdit { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; "
        "border-radius: 4px; padding: 8px; font-family: 'Consolas'; }"
        "QTextEdit::placeholder { color: #a16207; }"
    );
    layout->addWidget(new QLabel("Salida (Codigo C++):"));
    layout->addWidget(outputTextEdit);
    
    statusTextEdit = new QTextEdit();
    statusTextEdit->setMaximumHeight(150);
    statusTextEdit->setPlaceholderText("Estado del procesamiento...");
    statusTextEdit->setStyleSheet(
        "QTextEdit { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; "
        "border-radius: 4px; padding: 8px; font-family: 'Consolas'; }"
        "QTextEdit::placeholder { color: #a16207; }"
    );
    layout->addWidget(new QLabel("Estado:"));
    layout->addWidget(statusTextEdit);
    
    instructionLabel = new QLabel("Instrucciones detectadas: 0");
    instructionLabel->setStyleSheet("color: #eab308; font-weight: bold;");
    layout->addWidget(instructionLabel);
}

void Proyecto_EstructurasH1::setupWindowProperties()
{
    setWindowTitle("C_HaroldA - Convertidor de Lenguaje Natural a C++");
    setMinimumSize(1200, 800);
    
    setStyleSheet(
        "QMainWindow { background-color: #0b0f14; color: #fde68a; }"
        "QMenuBar { background-color: #111317; color: #fde68a; border-bottom: 1px solid #f59e0b; }"
        "QMenuBar::item { background-color: transparent; padding: 4px 8px; }"
        "QMenuBar::item:selected { background-color: #f59e0b; color: #0b0f14; }"
        "QToolBar { background-color: #111317; border: 1px solid #f59e0b; }"
    );
    
    showMaximized();
}

void Proyecto_EstructurasH1::setupConnections()
{
    connect(inputTextEdit, &QTextEdit::textChanged, this, &Proyecto_EstructurasH1::onEntradaTextChanged);
    connect(newButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onNuevoClicked);
    connect(loadButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onCargarClicked);
    connect(processButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onProcesarClicked);
    connect(validateButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onValidarClicked);
    connect(saveButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onGuardarClicked);
}

void Proyecto_EstructurasH1::updateStatus(const std::string& message, bool isError)
{
    QString timestamp = QTime::currentTime().toString("[hh:mm:ss] ");
    QString fullMessage = timestamp + QString::fromStdString(message);
    
    if (isError) {
        fullMessage = "<span style='color: #ef4444; font-weight: bold;'>[ERROR]</span> " + fullMessage;
    } else {
        fullMessage = "<span style='color: #fbbf24; font-weight: bold;'>[OK]</span> " + fullMessage;
    }
    
    if (statusTextEdit) {
        statusTextEdit->append(fullMessage);
        QTextCursor cursor = statusTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        statusTextEdit->setTextCursor(cursor);
    }
    
    if (statusBar()) {
        statusBar()->showMessage(QString::fromStdString(message), 5000);
        statusBar()->setStyleSheet("QStatusBar { background-color: #111317; color: #fde68a; border-top: 1px solid #f59e0b; }");
    }
}

void Proyecto_EstructurasH1::updateInstructionCount()
{
    if (processor && instructionLabel) {
        int count = processor->getInstructionCount();
        instructionLabel->setText("Instrucciones detectadas: " + QString::number(count));
    }
}

void Proyecto_EstructurasH1::displayErrors()
{
    if (processor) {
        const LinkedList<std::string>& errs = processor->getErrors();
        
        if (errs.getSize() > 0) {
            updateStatus("--- ERRORES ---", true);
            for (int i = 0; i < errs.getSize(); i++) {
                updateStatus("[" + std::to_string(i+1) + "] " + errs.get(i), true);
            }
            updateStatus("--- FIN ERRORES ---", true);
        }
    }
}

void Proyecto_EstructurasH1::displaySyntaxErrors()
{
    displayErrors();
}

void Proyecto_EstructurasH1::newFile()
{
    onNuevoClicked();
}

void Proyecto_EstructurasH1::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

QPixmap Proyecto_EstructurasH1::createCircularAvatar(const QString& imagePath, int size)
{
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    return result;
}

// HTML de ayuda
static const char* kHelpHtml = R"HTML(
<div style="background:#0b0f14; color:#fde68a; font-family:'Consolas','Fira Code',monospace; padding:16px; line-height:1.35;">
  <h2 style="color:#fbbf24; margin:0 0 6px 0;">Guia del DSL de <b>C_HaroldA</b></h2>
  <p style="color:#eab308; margin:0 0 14px 0;">Escribe <b>una instruccion por linea</b>. El programa valida y convierte a C++.</p>

  <h3 style="color:#fbbf24;">1) Plantilla minima</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
comenzar programa
...tus instrucciones...
terminar programa
  </pre>

  <h3 style="color:#fbbf24;">2) Comentarios</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
comentario Esto es un comentario
  </pre>

  <h3 style="color:#fbbf24;">3) Variables</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
crear variable entero llamada edad valor 25
crear variable decimal llamada altura valor 1.75
crear variable texto llamada nombre valor Harold
crear variable caracter llamada inicial valor H
crear variable booleano llamada activo valor true
  </pre>

  <h3 style="color:#fbbf24;">4) Listas / Arreglos</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
crear lista de enteros con 10 elementos llamada numeros
asignar 99 a numeros en posicion 0
  </pre>

  <h3 style="color:#fbbf24;">5) Entrada / Salida</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
leer edad
mostrar edad
mostrar Bienvenido a "C_HaroldA"
  </pre>

  <h3 style="color:#fbbf24;">6) Operaciones Aritmeticas</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
sumar 5 y 10 y mostrar resultado
  </pre>

  <h3 style="color:#fbbf24;">7) Comparadores y Logicos</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
igual a, distinto de, diferente de
mayor que, menor que
mayor o igual que, menor o igual que
y (AND), o (OR), no (NOT)
  </pre>

  <h3 style="color:#fbbf24;">8) Control de flujo basico</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
si edad mayor que 18
    mostrar Es mayor de edad
sino
    mostrar Es menor de edad

mientras contador menor que 3
    sumar contador y 1

repetir hasta contador igual a 0
    mostrar listo

para cada numero en numeros
    mostrar numero
  </pre>

  <h3 style="color:#fbbf24;">9) Bucle FOR clasico (en espanol)</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
# Sintaxis 1: for i desde ... hasta ... paso ...
for i desde 0 hasta 9 paso 1
    mostrar i

# Sintaxis 2: para i desde ... hasta ... paso ...
para i desde 1 hasta 10 paso 2
    mostrar i
  </pre>
</div>
)HTML";

void Proyecto_EstructurasH1::showHelpDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Ayuda - C_HaroldA");
    dlg.setModal(true);
    dlg.resize(760, 640);
    
    dlg.setStyleSheet("QDialog { background-color: #0b0f14; color: #fde68a; }");

    auto* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(12,12,12,12);
    lay->setSpacing(10);

    auto* tb = new QTextBrowser(&dlg);
    tb->setOpenExternalLinks(false);
    tb->setStyleSheet(
        "QTextBrowser { background-color: #0b0f14; color: #fde68a; border: 2px solid #f59e0b; "
        "border-radius: 12px; padding: 8px; font-family: 'Consolas'; }"
        "QScrollBar:vertical { background-color: #111317; width: 12px; border-radius: 6px; }"
        "QScrollBar::handle:vertical { background-color: #f59e0b; border-radius: 6px; min-height: 20px; }"
    );
    tb->setHtml(kHelpHtml);

    auto* btnCerrar = new QPushButton("Cerrar", &dlg);
    btnCerrar->setCursor(Qt::PointingHandCursor);
    btnCerrar->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c); "
        "border: 1px solid #f59e0b; color: #0b0f14; font-weight: bold; padding: 8px 16px; "
        "border-radius: 6px; min-width: 80px; }"
    );
    connect(btnCerrar, &QPushButton::clicked, &dlg, &QDialog::accept);

    lay->addWidget(tb, 1);
    lay->addWidget(btnCerrar, 0, Qt::AlignRight);

    dlg.exec();
}
