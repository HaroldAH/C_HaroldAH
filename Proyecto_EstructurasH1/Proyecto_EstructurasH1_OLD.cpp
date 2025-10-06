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
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QEasingCurve>
#include <random>
#include <QPainter>
#include <QPixmap>

// --- helpers locales para efectos Zenitsu ---
static void attachZenitsuGlow(QPushButton* btn) {
    auto eff = new QGraphicsDropShadowEffect(btn);
    eff->setBlurRadius(16);
    eff->setOffset(0, 0);
    eff->setColor(QColor("#f59e0b"));
    btn->setGraphicsEffect(eff);

    auto up = new QPropertyAnimation(eff, "blurRadius", btn);
    up->setDuration(220);
    up->setStartValue(16);
    up->setEndValue(26);
    up->setEasingCurve(QEasingCurve::OutQuad);

    auto down = new QPropertyAnimation(eff, "blurRadius", btn);
    down->setDuration(220);
    down->setStartValue(26);
    down->setEndValue(16);
    down->setEasingCurve(QEasingCurve::OutQuad);

    QObject::connect(btn, &QPushButton::pressed,  btn, [up]()  { up->start();   });
    QObject::connect(btn, &QPushButton::released, btn, [down](){ down->start(); });
}

static QLabel* attachShineSweep(QPushButton* btn) {
    auto shine = new QLabel(btn);
    shine->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    shine->setFixedSize(btn->width()/3, btn->height());
    shine->hide();

    // crea franja diagonal translúcida (dorado suave)
    QPixmap pm(shine->size());
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    QLinearGradient g(0, 0, pm.width(), pm.height());
    g.setColorAt(0.0, QColor(255,248,225,  0));
    g.setColorAt(0.45, QColor(255,248,225,110)); // luz cálida
    g.setColorAt(0.55, QColor(251,191,36, 160)); // #fbbf24
    g.setColorAt(1.0, QColor(234, 88, 12,  0));  // #ea580c
    p.fillRect(pm.rect(), g);
    p.end();
    shine->setPixmap(pm);

    // animación de barrido
    auto anim = new QPropertyAnimation(shine, "pos", shine);
    anim->setDuration(600);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(btn, &QPushButton::pressed, btn, [btn, shine, anim](){
        shine->move(-shine->width(), 0);
        shine->show();
        anim->stop();
        anim->setStartValue(QPoint(-shine->width(), 0));
        anim->setEndValue(QPoint(btn->width(), 0));
        anim->start();
    });
    // auto-shine cada 10 s
    auto timer = new QTimer(btn);
    QObject::connect(timer, &QTimer::timeout, btn, [btn, shine, anim](){
        shine->move(-shine->width(), 0);
        shine->show();
        anim->stop();
        anim->setStartValue(QPoint(-shine->width(), 0));
        anim->setEndValue(QPoint(btn->width(), 0));
        anim->start();
    });
    timer->start(10000);
    return shine;
}

Proyecto_EstructurasH1::Proyecto_EstructurasH1(QWidget *parent)
    : QMainWindow(parent), processor(nullptr), isFileLoaded(false), isProcessed(false),
      inputTextEdit(nullptr), outputTextEdit(nullptr), statusTextEdit(nullptr),
      loadButton(nullptr), processButton(nullptr), validateButton(nullptr), 
      saveButton(nullptr), instructionLabel(nullptr),
      hasInput(false), hasGenerated(false), isValidated(false),
      isValid(false) // NUEVO
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
    
    // --- Rayito ? en la parte superior ---
    {
        auto bolt = new QLabel("?", this);
        bolt->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        bolt->setStyleSheet("color:#fbbf24; font-size:20px; font-weight:700;");
        auto op = new QGraphicsOpacityEffect(bolt);
        bolt->setGraphicsEffect(op);
        bolt->hide();

        auto animPos  = new QPropertyAnimation(bolt, "pos", this);
        auto animFade = new QPropertyAnimation(op,   "opacity", this);
        animPos->setDuration(900);
        animFade->setDuration(900);
        animFade->setStartValue(0.0);
        animFade->setKeyValueAt(0.25, 1.0);
        animFade->setEndValue(0.0);
        animPos->setEasingCurve(QEasingCurve::OutCubic);

        auto timer = new QTimer(this);
        QObject::connect(timer, &QTimer::timeout, this, [this, bolt, animPos, animFade]() {
            // posición inicial/final arriba del panel principal
            const int y = 8; // ajusta si hace falta
            const int startX = 12;
            const int endX   = width() - 24;
            bolt->move(startX, y);
            bolt->show();
            animPos->stop();  animFade->stop();
            animPos->setStartValue(QPoint(startX, y));
            animPos->setEndValue(QPoint(endX,   y));
            animPos->start(); animFade->start();
        });
        // intervalo aleatorio 8–13 s
        QObject::connect(timer, &QTimer::timeout, this, [timer]() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(8000, 13000);
            timer->setInterval(dis(gen));
        });
        timer->setInterval(10000);
        timer->start();
    }
    
    // --- Avatar Zenitsu en esquina superior derecha ---
    {
        zenitsuAvatar = new QLabel(this);
        zenitsuAvatar->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        zenitsuAvatar->setFixedSize(44, 44);
        
        QPixmap avatarPm = createCircularAvatar(":/img/zenitsu_avatar.png", 44);
        zenitsuAvatar->setPixmap(avatarPm);
        
        // Drop shadow suave
        auto shadow = new QGraphicsDropShadowEffect(zenitsuAvatar);
        shadow->setBlurRadius(8);
        shadow->setOffset(2, 2);
        shadow->setColor(QColor(245, 158, 11, 100)); // #f59e0b con alpha
        
        // Crear efecto de opacidad para pulso suave
        auto opacityEffect = new QGraphicsOpacityEffect(zenitsuAvatar);
        opacityEffect->setOpacity(1.0);
        
        // Combinar efectos: usar shadow como base y animar la opacidad del label
        zenitsuAvatar->setGraphicsEffect(shadow);
        
        auto pulseAnim = new QPropertyAnimation(zenitsuAvatar, "windowOpacity", this);
        pulseAnim->setDuration(2400);
        pulseAnim->setStartValue(1.0);
        pulseAnim->setKeyValueAt(0.5, 0.6);
        pulseAnim->setEndValue(1.0);
        pulseAnim->setEasingCurve(QEasingCurve::InOutSine);
        pulseAnim->setLoopCount(-1);
        pulseAnim->start();
        
        zenitsuAvatar->show();
    }
    
    // --- Watermark Zenitsu en esquina inferior derecha ---
    {
        zenitsuWatermark = new QLabel(this);
        zenitsuWatermark->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        zenitsuWatermark->setFixedSize(140, 140);
        
        QPixmap watermarkPm(":/img/zenitsu_silhouette.png");
        if (watermarkPm.isNull()) {
            // Fallback: crear silueta simple
            watermarkPm = QPixmap(140, 140);
            watermarkPm.fill(Qt::transparent);
            QPainter p(&watermarkPm);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(QColor("#fbbf24"), 2));
            p.setBrush(Qt::NoBrush);
            // Dibujar silueta estilizada de rayo
            QPainterPath lightning;
            lightning.moveTo(70, 20);
            lightning.lineTo(50, 60);
            lightning.lineTo(65, 60);
            lightning.lineTo(45, 120);
            lightning.lineTo(90, 70);
            lightning.lineTo(75, 70);
            lightning.lineTo(95, 20);
            lightning.closeSubpath();
            p.drawPath(lightning);
        } else {
            watermarkPm = watermarkPm.scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        zenitsuWatermark->setPixmap(watermarkPm);
        
        // Opacidad muy tenue
        auto watermarkOpacity = new QGraphicsOpacityEffect(zenitsuWatermark);
        watermarkOpacity->setOpacity(0.06);
        zenitsuWatermark->setGraphicsEffect(watermarkOpacity);
        
        zenitsuWatermark->show();
    }
    
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
    
    // Apply Zenitsu color palette to main window
    this->setStyleSheet(
        "QMainWindow { background-color: #0b0f14; }"
        "QWidget { background-color: #0b0f14; color: #fde68a; }"
        "QLabel { color: #fde68a; font-weight: bold; }"
    );
    
    // Input area
    inputTextEdit = new QTextEdit();
    inputTextEdit->setPlaceholderText("Contenido del archivo se mostrara aqui...");
    inputTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "}"
        "QTextEdit::placeholder {"
        "    color: #a16207;"
        "}"
    );
    layout->addWidget(new QLabel("Entrada (Lenguaje Natural):"));
    layout->addWidget(inputTextEdit);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    loadButton = new QPushButton("Cargar Archivo");
    processButton = new QPushButton("Procesar Conversion");
    validateButton = new QPushButton("Validar Sintaxis");
    saveButton = new QPushButton("Guardar Codigo");
    
    // Apply Zenitsu button styles
    QString buttonStyle = 
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
        "    color: #0b0f14;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fcd34d, stop:1 #f97316);"
        "    box-shadow: 0 2px 4px rgba(245,158,11,0.25);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ea580c, stop:1 #fbbf24);"
        "}"
        "QPushButton:disabled {"
        "    background-color: #111317;"
        "    color: #a16207;"
        "    border-color: #a16207;"
        "}";
    
    loadButton->setStyleSheet(buttonStyle);
    processButton->setStyleSheet(buttonStyle);
    validateButton->setStyleSheet(buttonStyle);
    saveButton->setStyleSheet(buttonStyle);
    
    // Apply Zenitsu glow effects
    attachZenitsuGlow(loadButton);
    attachZenitsuGlow(processButton);
    attachZenitsuGlow(validateButton);
    attachZenitsuGlow(saveButton);
    
    // Apply Zenitsu shine sweep effects
    attachShineSweep(loadButton);
    attachShineSweep(processButton);
    attachShineSweep(validateButton);
    attachShineSweep(saveButton);
    
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(saveButton);
    layout->addLayout(buttonLayout);
    
    // Output area
    outputTextEdit = new QTextEdit();
    outputTextEdit->setPlaceholderText("Codigo C++ generado aparecera aqui...");
    outputTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QTextEdit::placeholder {"
        "    color: #a16207;"
        "}"
    );
    layout->addWidget(new QLabel("Salida (Codigo C++):"));
    layout->addWidget(outputTextEdit);
    
    // Status area
    statusTextEdit = new QTextEdit();
    statusTextEdit->setMaximumHeight(150);
    statusTextEdit->setPlaceholderText("Estado del procesamiento...");
    statusTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QTextEdit::placeholder {"
        "    color: #a16207;"
        "}"
    );
    layout->addWidget(new QLabel("Estado:"));
    layout->addWidget(statusTextEdit);
    
    // Instruction count
    instructionLabel = new QLabel("Instrucciones detectadas: 0");
    instructionLabel->setStyleSheet("color: #eab308; font-weight: bold;");
    layout->addWidget(instructionLabel);
}

void Proyecto_EstructurasH1::setupWindowProperties()
{
    // Window title and properties
    setWindowTitle("C_HaroldA - Convertidor de Lenguaje Natural a C++");
    setMinimumSize(1200, 800);
    
    // Apply Zenitsu theme to window
    setStyleSheet(
        "QMainWindow {"
        "    background-color: #0b0f14;"
        "    color: #fde68a;"
        "}"
        "QMenuBar {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "    border-bottom: 1px solid #f59e0b;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 4px 8px;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #f59e0b;"
        "    color: #0b0f14;"
        "}"
        "QToolBar {"
        "    background-color: #111317;"
        "    border: 1px solid #f59e0b;"
        "}"
    );
    
    // Open window maximized for full screen experience
    showMaximized();
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
        // Apply Zenitsu theme to file dialog
        QFileDialog fileDialog(this);
        fileDialog.setWindowTitle("Cargar archivo de lenguaje natural - C_HaroldA");
        fileDialog.setNameFilter("Archivos de texto (*.txt);;Todos los archivos (*.*)");
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setStyleSheet(
            "QFileDialog {"
            "    background-color: #111317;"
            "    color: #fde68a;"
            "}"
            "QFileDialog QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
            "    color: #0b0f14;"
            "    border: 1px solid #f59e0b;"
            "    border-radius: 4px;"
            "    padding: 6px 12px;"
            "    font-weight: bold;"
            "}"
            "QFileDialog QListView {"
            "    background-color: #0b0f14;"
            "    color: #fde68a;"
            "    border: 1px solid #f59e0b;"
            "}"
            "QFileDialog QTreeView {"
            "    background-color: #0b0f14;"
            "    color: #fde68a;"
            "    border: 1px solid #f59e0b;"
            "}"
        );
        
        QString fileName;
        if (fileDialog.exec()) {
            QStringList fileNames = fileDialog.selectedFiles();
            if (!fileNames.isEmpty()) {
                fileName = fileNames.first();
            }
        }

        if (!fileName.isEmpty()) {
            isValid = false; // NUEVO
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

        // 1) Procesa con validaciones internas (DSL antes de generar)
        processor->processInstructions();

        // 2) Muestra el código (si se generó)
        std::string generatedCode = processor->getGeneratedCode();
        if (outputTextEdit) outputTextEdit->setPlainText(QString::fromStdString(generatedCode));

        // 3) Determina validez combinada (errores DSL + C++)
        bool cppOk = processor->validateSyntax(); // ahora NO borra errores del DSL
        const LinkedList<std::string>& errs = processor->getErrors(); // referencia
        isValid = (errs.getSize() == 0) && cppOk && !generatedCode.empty();

        isProcessed = true; // hubo intento de proceso
        enableButtons();

        if (!isValid) {
            updateStatus("Conversion con errores: no se puede compilar/guardar.", true);
            displayErrors();
        } else {
            updateStatus("Conversion validada. Codigo listo para compilar/guardar.");
        }

        updateStatus("Se procesaron " + std::to_string(processor->getInstructionCount()) + " instrucciones");
    } catch (const std::exception& e) {
        updateStatus(std::string("Error durante la conversion: ") + e.what(), true);
        isProcessed = false;
        isValid = false;
        enableButtons();
    } catch (...) {
        updateStatus("Error desconocido durante la conversion", true);
        isProcessed = false;
        isValid = false;
        enableButtons();
    }
}

void Proyecto_EstructurasH1::validateSyntax()
{
    if (!isFileLoaded) {
        updateStatus("Error: Debe cargar un archivo primero", true);
        return;
    }

    updateStatus("Iniciando validacion...", false);
    QApplication::processEvents();

    if (!isProcessed) {
        updateStatus("Debe procesar la conversion primero para validar", true);
        return;
    }

    // Validación simple del C++ generado
    std::string generatedCode = processor->getGeneratedCode();
    bool isCodeValid = !generatedCode.empty() && generatedCode.find("int main()") != std::string::npos;
    
    const LinkedList<std::string>& errs = processor->getErrors();
    isValid = (errs.getSize() == 0) && isCodeValid;

    if (isValid) {
        updateStatus("Validacion exitosa: codigo listo para compilar y guardar.", false);
    } else {
        updateStatus("Validacion fallida: hay errores o el codigo esta incompleto.", true);
        displayErrors();
    }
    enableButtons();
}

void Proyecto_EstructurasH1::saveCode()
{
    if (!isProcessed) {
        updateStatus("Error: Debe procesar la conversion primero", true);
        return;
    }

    // Verificacion final *antes* de abrir el dialogo
    const LinkedList<std::string>& errs = processor->getErrors(); // referencia constante
    bool cppOk = processor->validateSyntax();
    bool okToSave = (errs.getSize()==0) && cppOk && !processor->getGeneratedCode().empty();
    if (!okToSave) {
        updateStatus("No se puede guardar: el codigo tiene errores o esta vacio.", true);
        displayErrors();
        return;
    }

    // Apply Zenitsu theme to save dialog
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle("Guardar codigo C++ generado - C_HaroldA");
    fileDialog.setNameFilter("Archivos C++ (*.cpp);;Archivos de texto (*.txt);;Todos los archivos (*.*)");
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.selectFile("codigo_generado_HaroldA.cpp");
    fileDialog.setStyleSheet(
        "QFileDialog {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "}"
        "QFileDialog QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
        "    color: #0b0f14;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "}"
        "QFileDialog QListView {"
        "    background-color: #0b0f14;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "}"
        "QFileDialog QTreeView {"
        "    background-color: #0b0f14;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "}"
        "QFileDialog QLineEdit {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 3px;"
        "    padding: 4px;"
        "}"
    );
    
    QString fileName;
    if (fileDialog.exec()) {
        QStringList fileNames = fileDialog.selectedFiles();
        if (!fileNames.isEmpty()) {
            fileName = fileNames.first();
        }
    }

    if (!fileName.isEmpty()) {
        std::string filePath = fileName.toStdString();
        updateStatus("Guardando codigo C++ en: " + filePath);
        QApplication::processEvents();

        if (processor->saveToFile(filePath)) {
            updateStatus("Codigo C++ guardado exitosamente!");
            updateStatus("Archivo: " + filePath);
        } else {
            updateStatus("Error al guardar el archivo", true);
            displayErrors();
        }
    }
}

void Proyecto_EstructurasH1::showAbout()
{
    QString aboutText = 
        "C_HaroldA - Convertidor de Lenguaje Natural a C++\n"
        "Version: 1.0\n"
        "Desarrollado por: HAROLD AVIA HERNANDEZ\n"
        "Universidad Nacional - Sede Regional Brunca\n"
        "Campus: Perez Zeledon - Coto\n"
        "Curso: Estructura de Datos II - Ciclo 2025\n"
        "Profesores: Saray Castro - Hairol Romero";
    
    QMessageBox msgBox;
    msgBox.setWindowTitle("Acerca de C_HaroldA");
    msgBox.setText(aboutText);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "}"
        "QMessageBox QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
        "    color: #0b0f14;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "    min-width: 60px;"
        "}"
        "QMessageBox QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fcd34d, stop:1 #f97316);"
        "}"
    );
    msgBox.exec();
}

void Proyecto_EstructurasH1::showInstructions()
{
    QString instructionsText =
        "Manual de Usuario - C_HaroldA\n"
        "Pasos para usar el convertidor:\n"
        "1. Cargar Archivo: Haga clic en 'Cargar Archivo' y seleccione un archivo .txt\n"
        "2. Procesar: Haga clic en 'Procesar Conversion' para convertir las instrucciones\n"
        "3. Validar: Use 'Validar Sintaxis' para verificar que el codigo generado sea correcto\n"
        "4. Guardar: Guarde el codigo C++ generado usando 'Guardar Codigo'";
    
    QMessageBox msgBox;
    msgBox.setWindowTitle("Manual de Usuario - C_HaroldA");
    msgBox.setText(instructionsText);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #111317;"
        "    color: #fde68a;"
        "}"
        "QMessageBox QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c);"
        "    color: #0b0f14;"
        "    border: 1px solid #f59e0b;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "    min-width: 60px;"
        "}"
        "QMessageBox QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fcd34d, stop:1 #f97316);"
        "}"
    );
    msgBox.exec();
}

void Proyecto_EstructurasH1::updateStatus(const std::string& message, bool isError)
{
    QString timestamp = QTime::currentTime().toString("[hh:mm:ss] ");
    QString fullMessage = timestamp + QString::fromStdString(message);
    
    // Apply Zenitsu color coding to status messages
    if (isError) {
        fullMessage = "<span style='color: #ef4444; font-weight: bold;'>[ERROR]</span> " + fullMessage;
    } else {
        fullMessage = "<span style='color: #fbbf24; font-weight: bold;'>[OK]</span> " + fullMessage;
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
        // Apply Zenitsu style to status bar
        statusBar()->setStyleSheet(
            "QStatusBar {"

            "    background-color: #111317;"
            "    color: #fde68a;"
            "    padding: 4px;"
            "    border-top: 1px solid #f59e0b;"
            "}"
        );
    }
}

void Proyecto_EstructurasH1::displayErrors()
{
    if (processor) {
        const LinkedList<std::string>& errs = processor->getErrors(); // referencia, no copia
        
        if (errs.getSize() > 0) {
            updateStatus("--- ERRORES Y ADVERTENCIAS ---", true);
            for (int i = 0; i < errs.getSize(); i++) {
                updateStatus("[" + std::to_string(i+1) + "] " + errs.get(i), true);
            }
            updateStatus("--- FIN DE ERRORES ---", true);
        }
    }
}

void Proyecto_EstructurasH1::enableButtons()
{
    // Método legacy - ahora usar updateButtonStates()
    updateButtonStates();
}

void Proyecto_EstructurasH1::updateButtonStates()
{
    // Control de flujo de botones basado en flags
    if (processButton) processButton->setEnabled(hasInput);
    if (validateButton) validateButton->setEnabled(hasGenerated);
    if (saveButton) saveButton->setEnabled(isValidated);
}

void Proyecto_EstructurasH1::updateInstructionCount()
{
    if (processor && instructionLabel) {
        int count = processor->getInstructionCount();
        instructionLabel->setText("Instrucciones detectadas: " + QString::number(count));
    }
}

void Proyecto_EstructurasH1::resetInterface()
{
    // Reset simplificado - solo actualizar estados de botones
    hasInput = false;
    hasGenerated = false;
    isValidated = false;
    updateButtonStates();
}

void Proyecto_EstructurasH1::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    
    // Reposicionar avatar Zenitsu en esquina superior derecha
    if (zenitsuAvatar) {
        zenitsuAvatar->move(width() - 56, 12);
    }
    
    // Reposicionar watermark en esquina inferior derecha
    if (zenitsuWatermark) {
        zenitsuWatermark->move(width() - 152, height() - 152);
    }
}

// Helper para crear avatar circular
QPixmap Proyecto_EstructurasH1::createCircularAvatar(const QString& imagePath, int size)
{
    QPixmap source(imagePath);
    if (source.isNull()) {
        // Crear avatar por defecto con ?
        source = QPixmap(size, size);
        source.fill(Qt::transparent);
        QPainter p(&source);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#fbbf24"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, size, size);
        p.setPen(QColor("#0b0f14"));
        p.setFont(QFont("Arial", size/3, QFont::Bold));
        p.drawText(source.rect(), Qt::AlignCenter, "?");
    } else {
        source = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Crear máscara circular
    QPixmap circular(size, size);
    circular.fill(Qt::transparent);
    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(source));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);
    
    return circular;
}