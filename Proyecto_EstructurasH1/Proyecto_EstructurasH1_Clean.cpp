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
#include <QDialog>
#include <QTextBrowser>
#include <QShortcut>
#include <QKeySequence>
#include <QDockWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QCompleter>
#include <QStringList>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <atomic>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSaveFile>

// Guardia RAII para prevenir reentrancia
struct BusyGuard {
    std::atomic<bool>& flag;
    bool wasSet;
    
    BusyGuard(std::atomic<bool>& f) : flag(f), wasSet(false) {
        bool expected = false;
        if (!flag.compare_exchange_strong(expected, true)) {
            throw std::runtime_error("Operacion en curso - evitando reentrancia");
        }
        wasSet = true;
    }
    
    ~BusyGuard() {
        if (wasSet) {
            flag.store(false);
        }
    }
    
    BusyGuard(const BusyGuard&) = delete;
    BusyGuard& operator=(const BusyGuard&) = delete;
};

// Helpers globales para conversiones seguras
std::string toStdStringUtf8(const QString& qstr) {
    if (qstr.isNull()) return std::string();
    QByteArray ba = qstr.toUtf8();
    return std::string(ba.constData(), static_cast<size_t>(ba.size()));
}

std::string safeFromC(const char* cstr) {
    return cstr ? std::string(cstr) : std::string();
}

// Estructuras simples para el Asistente DSL
struct SnippetField {
    QString key;
    QString label;
    QString def;
    SnippetField(const QString& k, const QString& l, const QString& d) : key(k), label(l), def(d) {}
};

struct Snippet {
    QString id;
    QString title;
    QString category;
    QString tpl;
    QList<SnippetField> fields;
    
    Snippet(const QString& i, const QString& t, const QString& c, const QString& template_str) 
        : id(i), title(t), category(c), tpl(template_str) {}
    
    void addField(const QString& key, const QString& label, const QString& def = "") {
        fields.append(SnippetField(key, label, def));
    }
};

// HTML de la guía DSL para C_HaroldA
static const char* kHelpHtml = R"HTML(
<div style="background:#0b0f14; color:#fde68a; font-family:'Consolas','Fira Code',monospace; padding:16px; line-height:1.35;">
  <h2 style="color:#fbbf24; margin:0 0 6px 0;">Guía del DSL de <b>C_HaroldA</b> ?</h2>
  <p style="color:#eab308; margin:0 0 14px 0;">Escribe <b>una instrucción por línea</b>. El programa valida y convierte a C++.</p>

  <h3 style="color:#fbbf24;">1) Plantilla mínima</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
comenzar programa
...tus instrucciones...
terminar programa
  </pre>

  <h3 style="color:#fbbf24;">2) Comentarios</h3>
  <p>Las líneas que empiezan con <b>#</b> se ignoran.</p>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
# esto no se convierte a código
  </pre>

  <h3 style="color:#fbbf24;">3) Variables</h3>
  <p><code>crear variable &lt;tipo&gt; llamada &lt;nombre&gt; [valor &lt;inicial&gt;]</code></p>
  <ul>
    <li>Tipos: <code>entero</code>, <code>decimal</code>, <code>texto</code>, <code>caracter</code>, <code>booleano</code></li>
    <li>Ejemplos:</li>
  </ul>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
crear variable entero llamada edad valor 25
crear variable decimal llamada altura valor 1.75
crear variable texto llamada nombre valor Harold
crear variable caracter llamada inicial valor H
crear variable booleano llamada activo valor true
  </pre>

  <h3 style="color:#fbbf24;">4) Listas / Arreglos</h3>
  <p><code>crear lista|arreglo de &lt;tipo&gt; con &lt;N&gt; elementos llamada &lt;nombre&gt;</code></p>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
crear lista de enteros   con 10 elementos llamada numeros
crear arreglo de decimales con 5 elementos llamada promedios
crear lista de texto     con 8  elementos llamada nombres
crear arreglo de caracteres con 3 elementos llamada iniciales
  </pre>
  <p>Asignaciones:</p>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
asignar valor 0 a contador
asignar 99 a numeros en posicion 0
asignar nombre a nombres en posicion 2
  </pre>

  <h3 style="color:#fbbf24;">5) Entrada / Salida</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
leer edad
ingresar nombre
mostrar edad
imprimir nombre
mostrar Bienvenido a C_HaroldA
  </pre>

  <h3 style="color:#fbbf24;">6) Operaciones aritméticas</h3>
  <p>Puedes separar con <b>y</b> o comas.</p>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
sumar 5 y 10 y mostrar resultado
sumar 15, 30, 45
restar 100 y 25
multiplicar 8 y 9 y 2
dividir 144 entre 12
  </pre>

  <h3 style="color:#fbbf24;">7) Comparadores y lógicos (para condiciones)</h3>
  <ul>
    <li>Comparadores: <code>igual a</code>, <code>distinto de</code>, <code>mayor que</code>, <code>menor que</code>, <code>mayor o igual que</code>, <code>menor o igual que</code></li>
    <li>Lógicos: <code>y</code> (AND), <code>o</code> (OR), <code>no</code> (NOT)</li>
  </ul>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
si edad mayor o igual que 18 y activo igual a true
    mostrar Acceso permitido
sino
    mostrar Acceso denegado
  </pre>

  <h3 style="color:#fbbf24;">8) Control de flujo</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
# Condicional
si edad mayor que 18
    mostrar Es mayor de edad
sino
    mostrar Es menor de edad

# Bucle mientras
mientras contador menor que 3
    sumar contador y 1

# Repetir hasta
repetir hasta edad igual a 50
    sumar edad y 1

# Para cada (foreach)
para cada numero en numeros
    mostrar numero
  </pre>

  <h3 style="color:#fbbf24;">9) Bucle <span style="color:#fcd34d;">PORT</span> (atajos de bucles)</h3>
  <p>Soporta <b>3 formas habituales</b> en los trabajos de clase. Si tu versión no reconoce <code>PORT</code>, usa sus equivalentes de arriba (mientras / para cada / repetir).</p>
  <ol>
    <li><b>Conteo por rango</b> — equivalente a un <i>for</i> clásico:</li>
  </ol>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
PORT i desde 0 hasta 9 paso 1
    mostrar i
  </pre>
  <ol start="2">
    <li><b>Repeticiones fijas</b> — ejecuta N veces:</li>
  </ol>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
PORT 5 veces
    mostrar Hola
  </pre>
  <ol start="3">
    <li><b>Sobre colección</b> — sinónimo compacto de <code>para cada</code>:</li>
  </ol>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
PORT cada item en numeros
    mostrar item
  </pre>
  <p style="margin-top:6px;">Notas: <code>paso</code> es opcional (por defecto 1). Los nombres de índice/ítem son libres (<i>i</i>, <i>k</i>, <i>item</i>, etc.).</p>

  <h3 style="color:#fbbf24;">10) Reglas y buenas prácticas</h3>
  <ul>
    <li>Una instrucción por línea.</li>
    <li>No importa mayúsculas/minúsculas.</li>
    <li>Decimales con <b>punto</b> (ej. <code>3.5</code>), no con coma.</li>
    <li>En operaciones la versión actual acepta <b>enteros puros</b> (ej. <code>10</code>, <code>-3</code>).</li>
    <li>Si usas un nombre (variable/lista), primero debes <b>crearlo</b>.</li>
    <li>Usa <b>Validar Sintaxis</b> antes de guardar.</li>
  </ul>

  <h3 style="color:#fbbf24;">11) Errores comunes (y cómo evitarlos)</h3>
  <ul>
    <li><b>"El archivo está vacío"</b>: escribe al menos una línea entre comenzar/terminar.</li>
    <li><b>"valor decimal inválido"</b>: usa <code>1.75</code>, no <code>1,75</code>.</li>
    <li><b>"booleano debe ser true/false"</b>: solo <code>true</code> o <code>false</code>.</li>
    <li><b>Identificador inválido</b>: nombres deben iniciar con letra o <code>_</code> y solo contener letras/números/<code>_</code>.</li>
  </ul>

  <h3 style="color:#fbbf24;">12) Ejemplo completo</h3>
  <pre style="background:#111317; border:1px solid #f59e0b; padding:10px; border-radius:8px;">
comenzar programa
# Variables
crear variable entero  llamada edad    valor 25
crear variable texto   llamada nombre  valor Harold
crear variable booleano llamada activo valor true

# Lista y carga
crear lista de enteros con 3 elementos llamada numeros
asignar 10 a numeros en posicion 0
asignar 20 a numeros en posicion 1
asignar 30 a numeros en posicion 2

# Aritmética con salida
sumar 5 y 10 y mostrar resultado

# Bucle PORT (conteo)
PORT i desde 0 hasta 2
    mostrar i

# Foreach compacto
PORT cada n en numeros
    mostrar n

# Condición
si edad mayor o igual que 18 y activo igual a true
    mostrar Bienvenido
sino
    mostrar Permiso denegado
terminar programa
  </pre>

  <p style="margin-top:10px; color:#eab308;">Tip: si modificas el .txt en disco, la ventana recarga automáticamente. Vuelve a "Procesar Conversión" y luego "Validar Sintaxis".</p>
</div>
)HTML";


// Helpers locales para efectos Zenitsu
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

    QPixmap pm(shine->size());
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    QLinearGradient g(0, 0, pm.width(), pm.height());
    g.setColorAt(0.0, QColor(255,248,225,  0));
    g.setColorAt(0.45, QColor(255,248,225,110));
    g.setColorAt(0.55, QColor(251,191,36, 160));
    g.setColorAt(1.0, QColor(234, 88, 12,  0));
    p.fillRect(pm.rect(), g);
    p.end();
    shine->setPixmap(pm);

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

void Proyecto_EstructurasH1::showHelpDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Ayuda • C_HaroldA ?");
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
    QObject::connect(btnCerrar, &QPushButton::clicked, &dlg, &QDialog::accept);

    lay->addWidget(tb, 1);
    lay->addWidget(btnCerrar, 0, Qt::AlignRight);

    dlg.exec();
}

Proyecto_EstructurasH1::Proyecto_EstructurasH1(QWidget *parent)
    : QMainWindow(parent), processor(nullptr),
      inputTextEdit(nullptr), outputTextEdit(nullptr), statusTextEdit(nullptr),
      loadButton(nullptr), processButton(nullptr), validateButton(nullptr), 
      saveButton(nullptr), instructionLabel(nullptr),
      hasInput(false), hasGenerated(false), isValidated(false),
      ignoreNextChange(false), reloadPending(false), reloadTimer(nullptr),
      reloadEpoch(0)
{
    ui.setupUi(this);
    
    processor = new NaturalLanguageProcessor();
    
    createBasicUI();
    setupWindowProperties();
    setupConnections();
    resetInterface();
    createDSLAssistant();
    
    reloadTimer = new QTimer(this);
    reloadTimer->setSingleShot(false);
    reloadTimer->setInterval(500);
    connect(reloadTimer, &QTimer::timeout, this, &Proyecto_EstructurasH1::onReloadTimer);
    
    connect(&fileWatcher, &QFileSystemWatcher::fileChanged, 
            this, &Proyecto_EstructurasH1::onWatchedFileChanged, 
            Qt::UniqueConnection);
    
    QShortcut* helpShortcut = new QShortcut(QKeySequence::HelpContents, this);
    QObject::connect(helpShortcut, &QShortcut::activated, [this]() { showHelpDialog(); });
    
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
            const int y = 8;
            const int startX = 12;
            const int endX   = width() - 24;
            bolt->move(startX, y);
            bolt->show();
            animPos->stop();  animFade->stop();
            animPos->setStartValue(QPoint(startX, y));
            animPos->setEndValue(QPoint(endX,   y));
            animPos->start(); animFade->start();
        });
        
        QObject::connect(timer, &QTimer::timeout, this, [timer]() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(8000, 13000);
            timer->setInterval(dis(gen));
        });
        timer->setInterval(10000);
        timer->start();
    }
    
    updateStatus("Bienvenido a C_HaroldA - Convertidor de Lenguaje Natural a C++");
    updateStatus("Desarrollado por: HAROLD AVIA HERNANDEZ");
    updateStatus("Universidad Nacional - Sede Regional Brunca - Campus PZ-Coto");
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
    
    connect(inputTextEdit, &QTextEdit::textChanged, this, &Proyecto_EstructurasH1::onEntradaTextChanged, Qt::UniqueConnection);
    
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
    
    attachZenitsuGlow(newButton);
    attachZenitsuGlow(loadButton);
    attachZenitsuGlow(processButton);
    attachZenitsuGlow(validateButton);
    attachZenitsuGlow(saveButton);
    attachZenitsuGlow(helpButton);
    
    attachShineSweep(newButton);
    attachShineSweep(loadButton);
    attachShineSweep(processButton);
    attachShineSweep(validateButton);
    attachShineSweep(saveButton);
    attachShineSweep(helpButton);
    
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(helpButton);
    layout->addLayout(buttonLayout);
    
    QObject::connect(helpButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onAyudaClicked, Qt::UniqueConnection);
    QObject::connect(newButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onNuevoClicked, Qt::UniqueConnection);
    QObject::connect(loadButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onCargarClicked, Qt::UniqueConnection);
    QObject::connect(processButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onProcesarClicked, Qt::UniqueConnection);
    QObject::connect(validateButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onValidarClicked, Qt::UniqueConnection);
    QObject::connect(saveButton, &QPushButton::clicked, this, &Proyecto_EstructurasH1::onGuardarClicked, Qt::UniqueConnection);
    
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
    // Conexiones ya establecidas en createBasicUI() con Qt::UniqueConnection
}
// PARTE 2: Métodos principales (loadFile, processConversion, validateSyntax, saveCode)

void Proyecto_EstructurasH1::loadFile()
{
    try {
        QFileDialog fileDialog(this);
        fileDialog.setWindowTitle("Cargar archivo - C_HaroldA");
        fileDialog.setNameFilter("Archivos de texto (*.txt);;Todos (*.*)");
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setStyleSheet(
            "QFileDialog { background-color: #111317; color: #fde68a; }"
            "QFileDialog QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c); "
            "color: #0b0f14; border: 1px solid #f59e0b; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
            "QFileDialog QListView, QFileDialog QTreeView { background-color: #0b0f14; color: #fde68a; border: 1px solid #f59e0b; }"
        );
        
        QString fileName;
        if (fileDialog.exec()) {
            QStringList fileNames = fileDialog.selectedFiles();
            if (!fileNames.isEmpty()) {
                fileName = fileNames.first();
            }
        }

        if (!fileName.isEmpty()) {
            // PASO 1: Desregistrar TODAS las rutas previas (evita señales viejas)
            QStringList watchedFiles = fileWatcher.files();
            if (!watchedFiles.isEmpty()) {
                fileWatcher.removePaths(watchedFiles);
                updateStatus("Dejando de vigilar archivos anteriores", false);
            }
            
            // PASO 2: Incrementar epoch (descarta recargas viejas)
            reloadEpoch++;
            
            // PASO 3: Actualizar rutas actuales
            currentFilePath = toStdStringUtf8(fileName);
            currentFilePathQt = fileName;
            
            updateStatus("Cargando archivo: " + currentFilePath);

            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QByteArray bytes = file.readAll();
                file.close();
                
                // UTF-8 primero, fallback a Local8Bit
                QString content = QString::fromUtf8(bytes);
                if (content.isNull() || content.isEmpty()) {
                    content = QString::fromLocal8Bit(bytes);
                    updateStatus("Archivo leido con codificacion local", false);
                }
                
                if (inputTextEdit) {
                    {
                        QSignalBlocker blockEntrada(inputTextEdit);
                        inputTextEdit->setPlainText(content);
                    }
                    
                    try {
                        if (processor) processor->reset();
                    }
                    catch (...) {
                        updateStatus("Error durante reset", true);
                    }
                    
                    {
                        QSignalBlocker blockSalida(outputTextEdit);
                        if (outputTextEdit) outputTextEdit->clear();
                    }
                    
                    hasInput = !content.trimmed().isEmpty();
                    hasGenerated = false;
                    isValidated = false;
                    updateButtonStates();
            
                    // PASO 4: Registrar en watcher DESPUÉS de cargar
                    if (fileWatcher.addPath(currentFilePathQt)) {
                        updateStatus("Vigilando cambios en: " + currentFilePath);
                    } else {
                        updateStatus("Advertencia: No se pudo vigilar", true);
                    }
                    
                    updateStatus("Archivo cargado exitosamente!");
                    updateStatus("Listo para procesar");
                } else {
                    updateStatus("Error: Editor no disponible", true);
                }
            } else {
                updateStatus("Error al abrir: " + toStdStringUtf8(fileName), true);
            }
        }
    } catch (const std::exception& e) {
        updateStatus("Excepcion al cargar: " + safeFromC(e.what()), true);
        hasInput = false;
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    } catch (...) {
        updateStatus("Excepcion desconocida al cargar", true);
        hasInput = false;
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    }
}

void Proyecto_EstructurasH1::processConversion()
{
    try {
        BusyGuard busyGuard(isProcessing);
        
        if (!processor) {
            updateStatus("Error: Procesador no inicializado", true);
            return;
        }

        const QString editorText = inputTextEdit ? inputTextEdit->toPlainText().trimmed() : "";
        if (editorText.isEmpty()) {
            updateStatus("Error: No hay entrada", true);
            return;
        }

        updateStatus(QString("Procesando (%1 caracteres)...").arg(editorText.length()).toStdString(), false);

        {
            QSignalBlocker blockSalida(outputTextEdit);
            if (outputTextEdit) outputTextEdit->clear();
        }

        try {
            processor->reset();
        }
        catch (const std::exception& e) {
            updateStatus("Error durante reset: " + safeFromC(e.what()), true);
            return;
        }
        catch (...) {
            updateStatus("Error desconocido durante reset", true);
            return;
        }

        updateStatus("Analizando instrucciones...");

        const std::string inputText = toStdStringUtf8(editorText);
        
        bool success = false;
        try {
            success = processor->processText(inputText);
        }
        catch (const std::exception& e) {
            updateStatus("Error durante procesamiento: " + safeFromC(e.what()), true);
            success = false;
        }
        catch (...) {
            updateStatus("Error desconocido durante procesamiento", true);
            success = false;
        }

        std::string generatedCode;
        try {
            generatedCode = processor->getGeneratedCode();
        }
        catch (...) {
            generatedCode = "// Error al obtener código\n";
        }

        {
            QSignalBlocker blockSalida(outputTextEdit);
            if (outputTextEdit) {
                outputTextEdit->setPlainText(QString::fromStdString(generatedCode));
            }
        }

        hasInput = true;
        hasGenerated = !generatedCode.empty();
        isValidated = false;
        updateButtonStates();

        updateStatus(QString("Código generado (%1 caracteres)").arg(generatedCode.length()).toStdString(), false);
        
        if (hasGenerated) {
            if (success) {
                updateStatus("Codigo generado exitosamente");
            } else {
                updateStatus("Conversion con advertencias", true);
            }
        } else {
            updateStatus("No se pudo generar codigo", true);
        }

        try {
            updateStatus("Procesadas " + std::to_string(processor->getInstructionCount()) + " instrucciones");
        }
        catch (...) {
        }
        
    }
    catch (const std::runtime_error& e) {
        std::string msg = safeFromC(e.what());
        if (msg.find("reentrancia") != std::string::npos || msg.find("curso") != std::string::npos) {
            updateStatus("Procesamiento ya en curso", true);
        } else {
            updateStatus("Error: " + msg, true);
        }
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    }
    catch (const std::exception& e) {
        updateStatus("Excepcion: " + safeFromC(e.what()), true);
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    }
    catch (...) {
        updateStatus("Excepcion desconocida", true);
        hasGenerated = false;
        isValidated = false;
        updateButtonStates();
    }
}

void Proyecto_EstructurasH1::validateSyntax()
{
    try {
        updateStatus("Iniciando validacion...", false);

        QString generatedCode = outputTextEdit ? outputTextEdit->toPlainText().trimmed() : "";
        if (generatedCode.isEmpty()) {
            updateStatus("Error: No hay codigo generado", true);
            isValidated = false;
            updateButtonStates();
            return;
        }

        try {
            bool isCodeValid = !generatedCode.isEmpty() && generatedCode.contains("int main()");
            
            const LinkedList<std::string>& errs = processor->getErrors();
            bool noErrors = (errs.getSize() == 0);
            
            isValidated = isCodeValid && noErrors;
            updateButtonStates();

            if (isValidated) {
                updateStatus("Validacion exitosa", false);
            } else {
                updateStatus("Validacion fallida", true);
                displayErrors();
            }
            
        } catch (const std::exception& e) {
            updateStatus("Error durante validacion: " + safeFromC(e.what()), true);
            isValidated = false;
            updateButtonStates();
        } catch (...) {
            updateStatus("Error desconocido durante validacion", true);
            isValidated = false;
            updateButtonStates();
        }
        
    } catch (const std::exception& e) {
        updateStatus("Excepcion: " + safeFromC(e.what()), true);
        isValidated = false;
        updateButtonStates();
    } catch (...) {
        updateStatus("Excepcion desconocida", true);
        isValidated = false;
        updateButtonStates();
    }
}

void Proyecto_EstructurasH1::saveCode()
{
    try {
        if (!isValidated) {
            updateStatus("Error: Debe validar primero", true);
            return;
        }

        QString generatedCode = outputTextEdit ? outputTextEdit->toPlainText().trimmed() : "";
        if (generatedCode.isEmpty()) {
            updateStatus("Error: No hay codigo", true);
            return;
        }

        QFileDialog fileDialog(this);
        fileDialog.setWindowTitle("Guardar C++ - C_HaroldA");
        fileDialog.setNameFilter("C++ (*.cpp);;Texto (*.txt);;Todos (*.*)");
        fileDialog.setFileMode(QFileDialog::AnyFile);
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        fileDialog.selectFile("codigo_generado_HaroldA.cpp");
        fileDialog.setStyleSheet(
            "QFileDialog { background-color: #111317; color: #fde68a; }"
            "QFileDialog QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c); "
            "color: #0b0f14; border: 1px solid #f59e0b; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
            "QFileDialog QListView, QFileDialog QTreeView { background-color: #0b0f14; color: #fde68a; border: 1px solid #f59e0b; }"
            "QFileDialog QLineEdit { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; border-radius: 3px; padding: 4px; }"
        );
        
        QString fileName;
        if (fileDialog.exec()) {
            QStringList fileNames = fileDialog.selectedFiles();
            if (!fileNames.isEmpty()) {
                fileName = fileNames.first();
            }
        }

        if (!fileName.isEmpty()) {
            try {
                updateStatus("Guardando en: " + toStdStringUtf8(fileName));

                bool isSavingWatchedFile = (fileName == currentFilePathQt);
                
                if (isSavingWatchedFile) {
                    ignoreNextChange = true;
                }

                QSaveFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream stream(&file);
                    stream << generatedCode;
                    
                    if (file.commit()) {
                        updateStatus("Guardado exitosamente!");
                        updateStatus("Archivo: " + toStdStringUtf8(fileName));
                    } else {
                        updateStatus("Error al confirmar escritura", true);
                        if (isSavingWatchedFile) {
                            ignoreNextChange = false;
                        }
                    }
                } else {
                    updateStatus("Error al abrir para escritura", true);
                    if (isSavingWatchedFile) {
                        ignoreNextChange = false;
                    }
                }
                
            } catch (const std::exception& e) {
                updateStatus("Error guardando: " + safeFromC(e.what()), true);
                ignoreNextChange = false;
            } catch (...) {
                updateStatus("Error desconocido guardando", true);
                ignoreNextChange = false;
            }
        }
        
    } catch (const std::exception& e) {
        updateStatus("Excepcion: " + safeFromC(e.what()), true);
    } catch (...) {
        updateStatus("Excepcion desconocida", true);
    }
}
// PARTE 3: File Watching, Helpers y Slots

void Proyecto_EstructurasH1::onWatchedFileChanged(const QString& path)
{
    try {
        if (ignoreNextChange) {
            ignoreNextChange = false;
            updateStatus("Cambio ignorado (guardado desde app)", false);
            return;
        }
        
        if (path != currentFilePathQt) {
            return;
        }
        
        updateStatus("Detectado cambio: " + toStdStringUtf8(path), false);
        
        reloadPending = true;
        pendingReloadEpoch = reloadEpoch;
        
        if (reloadTimer) {
            reloadTimer->start();
        }
        
    } catch (const std::exception& e) {
        updateStatus("Error en onWatchedFileChanged: " + safeFromC(e.what()), true);
    } catch (...) {
        updateStatus("Error desconocido en onWatchedFileChanged", true);
    }
}

void Proyecto_EstructurasH1::onReloadTimer()
{
    try {
        if (!reloadPending) {
            if (reloadTimer) reloadTimer->stop();
            return;
        }
        
        // Descartar recarga si epoch cambió (archivo diferente cargado)
        if (pendingReloadEpoch != reloadEpoch) {
            reloadPending = false;
            if (reloadTimer) reloadTimer->stop();
            updateStatus("Recarga cancelada (archivo cambiado)", false);
            return;
        }
        
        if (isProcessing.load()) {
            updateStatus("Esperando procesamiento...", false);
            return;
        }
        
        if (currentFilePathQt.isEmpty()) {
            reloadPending = false;
            if (reloadTimer) reloadTimer->stop();
            return;
        }
        
        updateStatus("Recargando: " + toStdStringUtf8(currentFilePathQt), false);
        
        QFile file(currentFilePathQt);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            updateStatus("Archivo bloqueado, reintentando...", false);
            return;
        }
        
        QByteArray bytes = file.readAll();
        file.close();
        
        QString content = QString::fromUtf8(bytes);
        if (content.isNull() || content.isEmpty()) {
            content = QString::fromLocal8Bit(bytes);
        }
        
        if (content.isNull()) {
            updateStatus("Contenido null, esperando...", false);
            return;
        }
        
        if (inputTextEdit) {
            {
                QSignalBlocker blockEntrada(inputTextEdit);
                inputTextEdit->setPlainText(content);
            }
            
            try {
                if (processor) processor->reset();
            }
            catch (...) {
                updateStatus("Error durante reset en recarga", true);
            }
            
            {
                QSignalBlocker blockSalida(outputTextEdit);
                if (outputTextEdit) outputTextEdit->clear();
            }
            
            hasInput = !content.trimmed().isEmpty();
            hasGenerated = false;
            isValidated = false;
            updateButtonStates();
            
            updateStatus("Recargado exitosamente!", false);
        }
        
        reloadPending = false;
        if (reloadTimer) reloadTimer->stop();
        
        if (!fileWatcher.files().contains(currentFilePathQt)) {
            if (fileWatcher.addPath(currentFilePathQt)) {
                updateStatus("Re-registrado en vigilancia", false);
            }
        }
        
    } catch (const std::exception& e) {
        updateStatus("Error durante recarga: " + safeFromC(e.what()), true);
        reloadPending = false;
        if (reloadTimer) reloadTimer->stop();
    } catch (...) {
        updateStatus("Error desconocido durante recarga", true);
        reloadPending = false;
        if (reloadTimer) reloadTimer->stop();
    }
}

void Proyecto_EstructurasH1::newFile()
{
    try {
        QStringList watchedFiles = fileWatcher.files();
        if (!watchedFiles.isEmpty()) {
            fileWatcher.removePaths(watchedFiles);
            updateStatus("Dejando de vigilar", false);
        }
        
        reloadEpoch++;
        
        currentFilePath.clear();
        currentFilePathQt.clear();
        
        if (inputTextEdit) {
            {
                QSignalBlocker blockEntrada(inputTextEdit);
                inputTextEdit->clear();
                inputTextEdit->setPlainText("comenzar programa\n\nterminar programa\n");
            }
            
            try {
                if (processor) processor->reset();
            }
            catch (...) {
                updateStatus("Error durante reset", true);
            }
            
            {
                QSignalBlocker blockSalida(outputTextEdit);
                if (outputTextEdit) outputTextEdit->clear();
            }
            
            hasInput = true;
            hasGenerated = false;
            isValidated = false;
            updateButtonStates();
            
            QTextCursor cursor = inputTextEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 1);
            cursor.movePosition(QTextCursor::EndOfLine);
            inputTextEdit->setTextCursor(cursor);
            
            inputTextEdit->setFocus();
        }
        
        if (instructionLabel) instructionLabel->setText("Instrucciones detectadas: 0");
        
        updateStatus("Nuevo archivo creado");
    }
    catch (const std::exception& e) {
        updateStatus("Excepcion en newFile: " + safeFromC(e.what()), true);
    }
    catch (...) {
        updateStatus("Excepcion desconocida en newFile", true);
    }
}

void Proyecto_EstructurasH1::showAbout()
{
    QString aboutText = 
        "C_HaroldA - Convertidor DSL a C++\n"
        "Version: 1.0\n"
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

QString Proyecto_EstructurasH1::processSnippetTemplate(const QString& templateStr, const QList<SnippetField>& fields, bool useDialog)
{
    Q_UNUSED(fields);
    Q_UNUSED(useDialog);
    return templateStr;
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

void Proyecto_EstructurasH1::updateButtonStates()
{
    if (processButton) processButton->setEnabled(hasInput);
    if (validateButton) validateButton->setEnabled(hasGenerated);
    if (saveButton) saveButton->setEnabled(isValidated);
}

void Proyecto_EstructurasH1::resetInterface()
{
    hasInput = false;
    hasGenerated = false;
    isValidated = false;
    updateButtonStates();
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

void Proyecto_EstructurasH1::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

// Slots
void Proyecto_EstructurasH1::onEntradaTextChanged()
{
    hasInput = !inputTextEdit->toPlainText().trimmed().isEmpty();
    hasGenerated = false;
    isValidated = false;
    updateButtonStates();
}

void Proyecto_EstructurasH1::onNuevoClicked() { newFile(); }
void Proyecto_EstructurasH1::onCargarClicked() { loadFile(); }
void Proyecto_EstructurasH1::onProcesarClicked() { processConversion(); }
void Proyecto_EstructurasH1::onValidarClicked() { validateSyntax(); }
void Proyecto_EstructurasH1::onGuardarClicked() { saveCode(); }
void Proyecto_EstructurasH1::onAyudaClicked() { showHelpDialog(); }
// PARTE 4: DSL Assistant

void Proyecto_EstructurasH1::createDSLAssistant()
{
    dslAssistantDock = new QDockWidget("Asistente DSL ?", this);
    dslAssistantDock->setStyleSheet(
        "QDockWidget { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; }"
        "QDockWidget::title { background-color: #111317; color: #fbbf24; font-weight: bold; padding: 4px; }"
    );

    QWidget* dockContent = new QWidget();
    dockContent->setStyleSheet("QWidget { background-color: #111317; color: #fde68a; }");

    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->setContentsMargins(8, 8, 8, 8);
    dockLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel("Plantillas DSL");
    titleLabel->setStyleSheet("color: #fbbf24; font-weight: bold;");
    dockLayout->addWidget(titleLabel);

    snippetTree = new QTreeWidget();
    snippetTree->setHeaderHidden(true);
    snippetTree->setRootIsDecorated(true);
    snippetTree->setStyleSheet(
        "QTreeWidget { background-color: #0b0f14; color: #fde68a; border: 1px solid #f59e0b; border-radius: 4px; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:hover { background-color: #1a1d22; }"
        "QTreeWidget::item:selected { background-color: #1a1d22; color: #fbbf24; }"
    );

    connect(snippetTree, &QTreeWidget::itemDoubleClicked, this, &Proyecto_EstructurasH1::onSnippetDoubleClick);

    dockLayout->addWidget(snippetTree);

    QPushButton* insertBtn = new QPushButton("Insertar");
    insertBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c); "
        "color: #0b0f14; border: 1px solid #f59e0b; border-radius: 4px; padding: 4px 8px; font-weight: bold; }"
    );

    connect(insertBtn, &QPushButton::clicked, [this]() {
        QTreeWidgetItem* current = snippetTree->currentItem();
        if (current && current->data(0, Qt::UserRole).isValid()) {
            QString snippetId = current->data(0, Qt::UserRole).toString();
            insertSnippet(snippetId, false);
        }
    });

    dockLayout->addWidget(insertBtn);

    dslAssistantDock->setWidget(dockContent);
    addDockWidget(Qt::RightDockWidgetArea, dslAssistantDock);

    setupSnippets();
    setupAutoCompleter();
}

void Proyecto_EstructurasH1::setupSnippets()
{
    snippetTree->clear();

    // Categorías
    auto* catVars  = new QTreeWidgetItem(snippetTree);  catVars->setText(0, "Variables");  catVars->setExpanded(true);  catVars->setFlags(Qt::ItemIsEnabled);
    auto* catOps   = new QTreeWidgetItem(snippetTree);  catOps->setText(0, "Operaciones"); catOps->setExpanded(false);  catOps->setFlags(Qt::ItemIsEnabled);
    auto* catIO    = new QTreeWidgetItem(snippetTree);  catIO->setText(0, "Entrada / Salida"); catIO->setExpanded(false); catIO->setFlags(Qt::ItemIsEnabled);
    auto* catList  = new QTreeWidgetItem(snippetTree);  catList->setText(0, "Listas / Arreglos"); catList->setExpanded(false); catList->setFlags(Qt::ItemIsEnabled);
    auto* catCtrl  = new QTreeWidgetItem(snippetTree);  catCtrl->setText(0, "Control");    catCtrl->setExpanded(true);  catCtrl->setFlags(Qt::ItemIsEnabled);
    auto* catTempl = new QTreeWidgetItem(snippetTree);  catTempl->setText(0, "Plantillas"); catTempl->setExpanded(true); catTempl->setFlags(Qt::ItemIsEnabled);

    // Helper function to add items
    auto add = [](QTreeWidgetItem* parent, const char* text, const char* id){
        auto* it = new QTreeWidgetItem(parent);
        it->setText(0, text);
        it->setData(0, Qt::UserRole, QString::fromUtf8(id));
        it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return it;
    };

    // Variables
    add(catVars,  "Variable Entero",   "var_int");
    add(catVars,  "Variable Decimal",  "var_dec");
    add(catVars,  "Variable Texto",    "var_text");
    add(catVars,  "Variable Caracter", "var_char");
    add(catVars,  "Variable Booleana", "var_bool");

    // Operaciones
    add(catOps, "Sumar y Mostrar",   "op_sum_print");
    add(catOps, "Sumar",             "op_sum");
    add(catOps, "Restar",            "op_sub");
    add(catOps, "Multiplicar",       "op_mul");
    add(catOps, "Dividir",           "op_div");
    add(catOps, "Asignar valor a variable", "op_assign");

    // Entrada / Salida
    add(catIO, "Leer variable",          "io_leer");
    add(catIO, "Ingresar variable",      "io_ingresar");
    add(catIO, "Mostrar texto",          "io_mostrar_texto");
    add(catIO, "Mostrar variable",       "io_mostrar_var");

    // Listas / Arreglos
    add(catList, "Crear lista enteros",     "list_int");
    add(catList, "Crear lista decimales",   "list_dec");
    add(catList, "Crear lista texto",       "list_text");
    add(catList, "Crear lista caracteres",  "list_char");
    add(catList, "Asignar a lista (posicion)", "list_assign_pos");

    // Control
    add(catCtrl, "Si - Sino",         "ctrl_if_else");
    add(catCtrl, "Mientras",          "ctrl_while");
    add(catCtrl, "Repetir hasta",     "ctrl_repeat");
    add(catCtrl, "Para cada",         "ctrl_foreach");
    add(catCtrl, "For clásico",       "ctrl_for");

    // Plantillas
    add(catTempl, "Programa Básico",        "templ_basic");
    add(catTempl, "Carga de lista con contador", "templ_load_list");
}

void Proyecto_EstructurasH1::setupAutoCompleter()
{
    QStringList dslKeywords = {
        // estructura
        "comenzar", "programa", "terminar",
        // crear
        "crear", "variable", "lista", "arreglo",
        "entero", "decimal", "texto", "caracter", "booleano",
        "enteros", "decimales", "caracteres",
        "llamada", "valor", "elementos",
        // listas
        "posicion", "asignar",
        // io
        "leer", "ingresar", "mostrar", "imprimir",
        // operaciones
        "sumar", "restar", "multiplicar", "dividir",
        // control
        "si", "sino", "mientras", "repetir", "hasta", "para", "cada",
        "for", "desde", "paso", "PORT", "veces",
        "mayor", "menor", "igual", "distinto", "o", "y", "no"
    };
    
    dslCompleter = new QCompleter(dslKeywords, this);
    dslCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    dslCompleter->setCompletionMode(QCompleter::PopupCompletion);
    
    if (inputTextEdit) {
        inputTextEdit->installEventFilter(this);
    }
}

void Proyecto_EstructurasH1::onSnippetDoubleClick(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (item && item->data(0, Qt::UserRole).isValid()) {
        QString snippetId = item->data(0, Qt::UserRole).toString();
        insertSnippet(snippetId, true);
    }
}

void Proyecto_EstructurasH1::insertSnippet(const QString& snippetId, bool useDialog)
{
    Q_UNUSED(useDialog);
    
    QString t;

    // VARIABLES
    if      (snippetId == "var_int")   t = "crear variable entero llamada edad valor 0";
    else if (snippetId == "var_dec")   t = "crear variable decimal llamada altura valor 0.0";
    else if (snippetId == "var_text")  t = "crear variable texto llamada nombre valor Harold";
    else if (snippetId == "var_char")  t = "crear variable caracter llamada inicial valor H";
    else if (snippetId == "var_bool")  t = "crear variable booleano llamada activo valor true";

    // OPERACIONES
    else if (snippetId == "op_sum_print") t = "sumar 5 y 10 y mostrar resultado";
    else if (snippetId == "op_sum")       t = "sumar 10, 20, 30";
    else if (snippetId == "op_sub")       t = "restar 100 y 25";
    else if (snippetId == "op_mul")       t = "multiplicar 8 y 9 y 2";
    else if (snippetId == "op_div")       t = "dividir 144 entre 12";
    else if (snippetId == "op_assign")    t = "asignar valor 0 a contador";

    // ENTRADA / SALIDA
    else if (snippetId == "io_leer")          t = "leer edad";
    else if (snippetId == "io_ingresar")      t = "ingresar nombre";
    else if (snippetId == "io_mostrar_texto") t = "mostrar Bienvenido a C_HaroldA";
    else if (snippetId == "io_mostrar_var")   t = "mostrar edad";

    // LISTAS
    else if (snippetId == "list_int")       t = "crear lista de enteros con 5 elementos llamada numeros";
    else if (snippetId == "list_dec")       t = "crear lista de decimales con 3 elementos llamada promedios";
    else if (snippetId == "list_text")      t = "crear lista de texto con 4 elementos llamada nombres";
    else if (snippetId == "list_char")      t = "crear lista de caracteres con 3 elementos llamada iniciales";
    else if (snippetId == "list_assign_pos")t = "asignar 99 a numeros en posicion 0";

    // CONTROL
    else if (snippetId == "ctrl_if_else")
        t = "si edad mayor que 18\n    mostrar Es mayor de edad\nsino\n    mostrar Es menor de edad";
    else if (snippetId == "ctrl_while")
        t = "mientras contador menor que 3\n    sumar contador y 1";
    else if (snippetId == "ctrl_repeat")
        t = "repetir hasta edad igual a 50\n    sumar edad y 1";
    else if (snippetId == "ctrl_foreach")
        t = "para cada numero en numeros\n    mostrar numero";
    else if (snippetId == "ctrl_for")
        t = "for i desde 0 hasta 9 paso 1\n    mostrar i";

    // PLANTILLAS
    else if (snippetId == "templ_basic")
        t = "comenzar programa\ncrear variable entero llamada contador valor 0\nmostrar contador\nterminar programa";
    else if (snippetId == "templ_load_list")
        t = "comenzar programa\ncrear lista de enteros con 3 elementos llamada numeros\ncrear variable entero llamada contador valor 0\nmientras contador menor que 3\n    leer numero\n    asignar numero a numeros en posicion contador\n    sumar contador y 1\npara cada n en numeros\n    mostrar n\nterminar programa";
    else
        return;
    
    if (inputTextEdit && !t.isEmpty()) {
        QTextCursor c = inputTextEdit->textCursor();
        if (!inputTextEdit->toPlainText().isEmpty())
            c.insertText("\n");
        c.insertText(t);
        inputTextEdit->setTextCursor(c);
    }
}
