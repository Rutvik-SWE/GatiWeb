#include "MainWindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QUrl>
#include <QTextCursor>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QSettings>
#include <QKeySequence>
#include <QPalette>
#include <QColor>
#include <QSizePolicy>
#include <QRegularExpression>
#include <QProgressBar>
#include <QScrollArea>
#include <QListWidgetItem>
#include <QTextBrowser>
#include <QStyle>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QStandardPaths>

// Fake a standard browser user-agent so websites don't block us
static const QString UA = QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36");

// Available theme colors (Name|Hex)
static const QStringList NAMED_COLORS = {
    "Kesari (Saffron)|#ff9933", "Marigold|#f59e0b",       "Turmeric|#ffc30b",
    "Kumkum|#e3242b",           "Sindoor|#ff4500",        "Lotus|#ffb6c1",
    "Peacock|#005b9f",          "Indigo|#4b0082",         "Ashoka|#000080",
    "Banyan|#8b4513",           "Neem|#2e8b57",           "Mango|#ffc000",
    "Sandstone|#cc7722",        "Copper|#b87333",         "Bronze|#cd7f32",
    "Ivory|#fffff0",            "Monsoon Grey|#708090",   "Himalayan Blue|#5386e4",
    "Ganga White|#f8f9fa",      "Royal Rajput Gold|#cfb53b"
};

// Domains to block for the ad-shield
static const QStringList AD_BLOCKLIST = {
    "doubleclick.net","googlesyndication.com","googleadservices.com",
    "adserver","ads.yahoo","tracker.com","analytics","pagead",
    "adnxs.com","taboola.com","outbrain.com","quantserve.com"
};

// Pre-compile regex patterns. Qt 6 automatically optimizes these via PCRE2 natively!
static const QRegularExpression AD_BLOCK_REGEX(
    QStringLiteral("(doubleclick\\.net|googlesyndication\\.com|googleadservices\\.com|"
                   "adserver|ads\\.yahoo|tracker\\.com|analytics|pagead|"
                   "adnxs\\.com|taboola\\.com|outbrain\\.com|quantserve\\.com)"),
    QRegularExpression::CaseInsensitiveOption
    );

static const QRegularExpression META_RE(
    QStringLiteral(R"(<meta[^>]+>)"),
    QRegularExpression::CaseInsensitiveOption
    );

static const QRegularExpression URL_RE(
    QStringLiteral(R"(url=['"]?([^"'>\s]+)['"]?)"),
    QRegularExpression::CaseInsensitiveOption
    );


// --- Window Setup & UI Construction ---
MainWindow::MainWindow(bool isIncognito, QWidget *parent)
    : QMainWindow(parent), m_isIncognito(isIncognito), m_sidebarExpanded(false), m_zoomFactor(1.0)
{
    if (m_isIncognito) setAttribute(Qt::WA_DeleteOnClose);
    m_customAccent = getCustomAccentColor();

    // Setup network manager and cookies
    networkManager = new QNetworkAccessManager(this);
    cookieJar = new QNetworkCookieJar(this);
    networkManager->setCookieJar(cookieJar);

    // Turn on disk caching to speed up page loads (skip if incognito)
    if (!m_isIncognito) {
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/GatiWebCache");
        diskCache->setCacheDirectory(cachePath);
        diskCache->setMaximumCacheSize(150 * 1024 * 1024); // 150 MB limit
        networkManager->setCache(diskCache);
    }

    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkData);

    // Main layout wrapper
    QWidget *central = new QWidget(this);
    QHBoxLayout *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    // Build the left sidebar
    leftPanel = new QWidget(central);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(64);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 16, 8, 16);
    leftLayout->setSpacing(16);

    sidebarToggleButton = new QPushButton("☰", leftPanel);
    sidebarToggleButton->setObjectName("sidebarToggle");
    sidebarToggleButton->setFixedSize(40, 40);
    sidebarToggleButton->setCursor(Qt::PointingHandCursor);
    leftLayout->addWidget(sidebarToggleButton);

    newTabButton = new QPushButton("✨", leftPanel);
    newTabButton->setObjectName("newTabBtn");
    newTabButton->setFixedHeight(40);
    newTabButton->setCursor(Qt::PointingHandCursor);
    newTabButton->setToolTip("New Chat / Tab (Ctrl+T)");
    leftLayout->addWidget(newTabButton);

    tabDock = new QListWidget(leftPanel);
    tabDock->setObjectName("tabDock");
    tabDock->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tabDock->setFocusPolicy(Qt::NoFocus);
    leftLayout->addWidget(tabDock, 1);

    settingsButton = new QPushButton("⚙️", leftPanel);
    settingsButton->setObjectName("settingsBtn");
    settingsButton->setFixedHeight(40);
    settingsButton->setCursor(Qt::PointingHandCursor);
    settingsButton->setToolTip("Settings (Ctrl+,)");
    leftLayout->addWidget(settingsButton);

    profileButton = new QPushButton("👤", leftPanel);
    profileButton->setObjectName("profileBtn");
    profileButton->setFixedHeight(40);
    profileButton->setCursor(Qt::PointingHandCursor);
    profileButton->setToolTip("User Profile");
    leftLayout->addWidget(profileButton);

    rootLayout->addWidget(leftPanel);

    // Build the main browser area (right side)
    QWidget *rightArea = new QWidget(central);
    rightArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *toolbar = new QHBoxLayout();
    toolbar->setSpacing(4);

    // Top navigation buttons
    backButton = new QPushButton("←", rightArea);
    forwardButton = new QPushButton("→", rightArea);
    refreshButton = new QPushButton("↻", rightArea);
    QPushButton *homeButton = new QPushButton("🏠", rightArea);

    for (auto *b : {backButton, forwardButton, refreshButton, homeButton}) {
        b->setObjectName("navBtn");
        b->setFixedSize(36, 36);
        b->setCursor(Qt::PointingHandCursor);
        toolbar->addWidget(b);
    }

    secureIcon = new QLabel("🔓", rightArea);
    secureIcon->setFixedSize(22, 36);
    secureIcon->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(secureIcon);

    addressOrb = new QLineEdit(rightArea);
    addressOrb->setObjectName("addressOrb");
    addressOrb->setPlaceholderText(m_isIncognito ? " 🕵 Search privately…" : " Ask GATIWEB or enter address…");
    addressOrb->setFixedHeight(36);
    toolbar->addWidget(addressOrb, 1);

    // Zoom controls
    zoomOutBtn = new QPushButton("−", rightArea);
    zoomLabel = new QLabel("100%", rightArea);
    zoomInBtn = new QPushButton("＋", rightArea);

    for (auto *b : {zoomOutBtn, zoomInBtn}) { b->setObjectName("zoomBtn"); b->setFixedSize(28, 28); b->setCursor(Qt::PointingHandCursor); }
    zoomLabel->setObjectName("zoomLabel"); zoomLabel->setFixedWidth(40); zoomLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(zoomOutBtn); toolbar->addWidget(zoomLabel); toolbar->addWidget(zoomInBtn);

    // Page tools (source, bookmarks, incognito)
    sourceButton = new QPushButton("{ }", rightArea);
    bookmarkButton = new QPushButton("☆", rightArea);
    incognitoButton = new QPushButton(m_isIncognito ? "●" : "◎", rightArea);

    bookmarkMenu = new QMenu(this);
    bookmarkButton->setMenu(bookmarkMenu);

    for (auto *b : {sourceButton, incognitoButton, bookmarkButton}) {
        b->setObjectName("navBtn"); b->setFixedSize(36, 36); b->setCursor(Qt::PointingHandCursor); toolbar->addWidget(b);
    }

    rightLayout->addLayout(toolbar);

    // Progress bar with smooth fade out effect
    loadProgress = new QProgressBar(rightArea);
    loadProgress->setObjectName("loadProgress");
    loadProgress->setRange(0, 100);
    loadProgress->setFixedHeight(3);
    loadProgress->setTextVisible(false);
    loadProgress->setVisible(false);

    QGraphicsOpacityEffect *progressEffect = new QGraphicsOpacityEffect(this);
    loadProgress->setGraphicsEffect(progressEffect);
    rightLayout->addWidget(loadProgress);

    // Holds the actual web pages
    contentStack = new QStackedWidget(rightArea);
    contentStack->setObjectName("contentStack");
    rightLayout->addWidget(contentStack, 1);

    // In-page search bar (hidden by default)
    QHBoxLayout *findRow = new QHBoxLayout();
    findBar = new QLineEdit(rightArea); findBar->setObjectName("findBar"); findBar->setVisible(false);
    findResultLabel = new QLabel("", rightArea); findResultLabel->setObjectName("findResultLabel"); findResultLabel->setVisible(false);
    findCloseButton = new QPushButton("✕", rightArea); findCloseButton->setObjectName("findCloseBtn"); findCloseButton->setFixedSize(28, 28); findCloseButton->setVisible(false);
    findRow->addWidget(findBar, 1); findRow->addWidget(findResultLabel); findRow->addWidget(findCloseButton);
    rightLayout->addLayout(findRow);

    rootLayout->addWidget(rightArea, 1);
    central->setLayout(rootLayout);
    setCentralWidget(central);

    // --- Connect Signals & Slots ---
    connect(sidebarToggleButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);
    connect(newTabButton,        &QPushButton::clicked, this, &MainWindow::addNewTab);
    connect(settingsButton,      &QPushButton::clicked, this, &MainWindow::openSettingsDialog);

    // Developer profile popup
    connect(profileButton, &QPushButton::clicked, this, [this]() {
        QDialog profDlg(this);
        profDlg.setWindowTitle("GATIWEB Profile Workspace");
        profDlg.setMinimumSize(520, 380);
        QVBoxLayout *pLayout = new QVBoxLayout(&profDlg);

        QLabel *pHeader = new QLabel("👤 Developer Profile Console", &profDlg);
        pHeader->setStyleSheet("font-size: 18px; font-weight: bold; color: " + m_customAccent + ";");
        pLayout->addWidget(pHeader);

        QGroupBox *pBox = new QGroupBox("Runtime Identity Parameters", &profDlg);
        pBox->setStyleSheet("QGroupBox { border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; padding-top: 20px; font-weight: bold; }");
        QVBoxLayout *boxL = new QVBoxLayout(pBox);

        auto addProfileField = [&](const QString &label, const QString &val) {
            QLabel *lbl = new QLabel(QStringLiteral("<b style='color: ") + m_customAccent + QStringLiteral(";'>%1:</b> %2").arg(label, val), pBox);
            lbl->setStyleSheet(QStringLiteral("font-size: 13px; color: #e2e8f0;")); lbl->setWordWrap(true); boxL->addWidget(lbl);
        };

        addProfileField("Engineer", "Rutvik Katariya");
        addProfileField("Target Roles", "Systems Software Engineer | C++ Developer | Software Architect");
        addProfileField("Core Stack", "C++ / Qt Framework / Java / Python / SQL");
        addProfileField("Specializations", "Systems Programming, Cross-Platform Architecture, Core Backend Systems, Optimization");
        addProfileField("Current Build", "GATIWEB Engine — Native C++ Web Browser");
        pLayout->addWidget(pBox);

        QDialogButtonBox *pBtns = new QDialogButtonBox(QDialogButtonBox::Close, &profDlg);
        pBtns->setStyleSheet(QStringLiteral("QPushButton { background: rgba(255,255,255,0.05); border-radius: 6px; padding: 6px 16px; color: #e2e8f0; } QPushButton:hover { background: ") + m_customAccent + QStringLiteral("; color: #000; }"));
        connect(pBtns, &QDialogButtonBox::rejected, &profDlg, &QDialog::reject);
        pLayout->addWidget(pBtns);
        profDlg.exec();
    });

    // Handle home button click
    connect(homeButton, &QPushButton::clicked, this, [this]() {
        auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
        if (!br) return;

        QString home = getHomePageUrl();
        if (!home.isEmpty() && home != "https://www.google.com") {
            addressOrb->setText(home);
            loadUrl();
        } else {
            addressOrb->clear();

            // ---> Inject the new local dashboard here <---
            br->setHtml(getDashboardHtml());

            if (auto *lbl = tabTitleLabel(br)) lbl->setText("New Session");
            updateSecureIcon(""); tabHistoryMap[br] = {}; tabHistoryIndexMap[br] = -1; updateNavigationButtons();
        }
    });

    // Map UI clicks to browser functions
    connect(backButton,    &QPushButton::clicked, this, &MainWindow::goBack);
    connect(forwardButton, &QPushButton::clicked, this, &MainWindow::goForward);
    connect(refreshButton, &QPushButton::clicked, this, [this]() { if (m_loading) stopLoading(); else refreshPage(); });
    connect(addressOrb, &QLineEdit::returnPressed, this, &MainWindow::loadUrl);
    connect(sourceButton,    &QPushButton::clicked, this, &MainWindow::viewPageSource);
    connect(incognitoButton, &QPushButton::clicked, this, &MainWindow::openIncognitoWindow);
    connect(zoomInBtn,  &QPushButton::clicked, this, &MainWindow::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &MainWindow::zoomOut);
    connect(zoomLabel,  &QLabel::linkActivated, this, &MainWindow::zoomReset);
    connect(findBar,         &QLineEdit::returnPressed, this, &MainWindow::findText);
    connect(findCloseButton, &QPushButton::clicked, this, [this]() { findBar->setVisible(false); findCloseButton->setVisible(false); findResultLabel->setVisible(false); });
    connect(tabDock, &QListWidget::itemClicked, this, &MainWindow::switchToTab);

    // Keyboard shortcuts
    auto sc = [this](QKeySequence ks, auto fn) { connect(new QShortcut(ks, this), &QShortcut::activated, this, fn); };
    sc(QKeySequence::Find, &MainWindow::showFindBar);
    sc(QKeySequence(Qt::CTRL|Qt::Key_T), &MainWindow::addNewTab);
    sc(QKeySequence(Qt::CTRL|Qt::Key_B), &MainWindow::toggleSidebar);
    sc(QKeySequence(Qt::CTRL|Qt::Key_Comma), &MainWindow::openSettingsDialog);
    sc(QKeySequence(Qt::CTRL|Qt::Key_U), &MainWindow::viewPageSource);
    sc(QKeySequence(Qt::Key_F5), &MainWindow::refreshPage);
    sc(QKeySequence(Qt::ALT|Qt::Key_Left), &MainWindow::goBack);
    sc(QKeySequence(Qt::ALT|Qt::Key_Right), &MainWindow::goForward);
    sc(QKeySequence(Qt::CTRL|Qt::Key_Plus), &MainWindow::zoomIn);
    sc(QKeySequence(Qt::CTRL|Qt::Key_Minus), &MainWindow::zoomOut);
    sc(QKeySequence(Qt::CTRL|Qt::Key_0), &MainWindow::zoomReset);
    sc(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_N), &MainWindow::openIncognitoWindow);
    sc(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_T), &MainWindow::undoClosedTab);

    // Initial setup
    applyGatiTheme();
    if (!m_isIncognito) updateBookmarkMenu();
    addNewTab();
}

MainWindow::~MainWindow() {}

// --- UI Animations ---

// Smoothly slide the sidebar open or closed
void MainWindow::toggleSidebar() {
    m_sidebarExpanded = !m_sidebarExpanded;

    // Hide text instantly before collapsing to avoid ugly clipping
    if (!m_sidebarExpanded) {
        newTabButton->setText("✨"); settingsButton->setText("⚙️"); profileButton->setText("👤");
        for (int i = 0; i < tabDock->count(); ++i) {
            if (auto *w = tabDock->itemWidget(tabDock->item(i))) {
                if (auto *lbl = w->findChild<QLabel*>("tabTitle")) lbl->setVisible(false);
                if (auto *btn = w->findChild<QPushButton*>("tabClose")) btn->setVisible(false);
            }
        }
    }

    // Animate the width change
    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setDuration(350); anim->setEasingCurve(QEasingCurve::OutExpo);
    anim->setStartValue(leftPanel->width()); anim->setEndValue(m_sidebarExpanded ? 240 : 64);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) { leftPanel->setFixedWidth(val.toInt()); });

    // Show text again once expansion finishes
    connect(anim, &QVariantAnimation::finished, this, [this]() {
        if (m_sidebarExpanded) {
            newTabButton->setText("✨ New Chat"); settingsButton->setText("⚙️ Settings"); profileButton->setText("👤 Profile");
            for (int i = 0; i < tabDock->count(); ++i) {
                if (auto *w = tabDock->itemWidget(tabDock->item(i))) {
                    if (auto *lbl = w->findChild<QLabel*>("tabTitle")) lbl->setVisible(true);
                    if (auto *btn = w->findChild<QPushButton*>("tabClose")) btn->setVisible(true);
                }
            }
        }
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Fade out the progress bar smoothly when loading finishes
void MainWindow::setLoadingState(bool loading) {
    m_loading = loading;
    QGraphicsOpacityEffect *eff = static_cast<QGraphicsOpacityEffect*>(loadProgress->graphicsEffect());
    if (!eff) { eff = new QGraphicsOpacityEffect(this); loadProgress->setGraphicsEffect(eff); }

    if (loading) {
        refreshButton->setText("✕"); loadProgress->setValue(0); eff->setOpacity(1.0); loadProgress->setVisible(true);
    } else {
        refreshButton->setText("↻"); loadProgress->setValue(100);
        QPropertyAnimation *fade = new QPropertyAnimation(eff, "opacity");
        fade->setDuration(500); fade->setStartValue(1.0); fade->setEndValue(0.0);
        connect(fade, &QPropertyAnimation::finished, [this, eff](){
            loadProgress->setVisible(false); loadProgress->setValue(0); eff->setOpacity(1.0);
        });
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

// --- Theme Engine ---

QString MainWindow::getCustomAccentColor() const {
    if (m_isIncognito) return "#f43f5e";
    return QSettings("GatiTeam", "GATIWEB").value("accent_color", "#ff9933").toString();
}

// Push the unified CSS rules to all UI components at once
void MainWindow::applyGatiTheme() {
    const QString accent = m_isIncognito ? "#f43f5e" : m_customAccent;
    const QString bg = "#131314"; const QString sidebar = "#1e1e20"; const QString surf = "#282a2c";
    const QString text = "#e2e8f0"; const QString dim = "#64748b";

    // Extract RGB from hex for alpha transparency
    QString hex = accent; hex.remove('#'); bool ok;
    int r = QStringView(hex).mid(0,2).toInt(&ok,16), g = QStringView(hex).mid(2,2).toInt(&ok,16), b = QStringView(hex).mid(4,2).toInt(&ok,16);
    const QString a12 = QStringLiteral("rgba(%1,%2,%3,0.12)").arg(r).arg(g).arg(b);

    this->setStyleSheet(QStringLiteral(
                            "QMainWindow, QDialog { background: %1; }"
                            "QGroupBox { border: 1px solid rgba(255,255,255,0.08); border-radius: 8px; color: %4; padding-top: 18px; }"
                            "QLabel, QCheckBox { color: %4; background: transparent; }"
                            "QComboBox { background: %3; border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; padding: 5px 10px; color: %4; }"
                            "QPushButton { background: %3; border: none; border-radius: 8px; color: %4; padding: 4px; }"
                            "QPushButton:hover { background: %6; color: %5; }"
                            "QLineEdit { background: %3; border: 1px solid rgba(255,255,255,0.08); border-radius: 10px; padding: 0 14px; color: %4; }"
                            "QLineEdit:focus { border-color: %5; }"
                            "QProgressBar { background: rgba(255,255,255,0.05); border: none; border-radius: 2px; }"
                            "QProgressBar::chunk { background: %5; border-radius: 2px; }"
                            "QWidget#leftPanel { background: %7; border-right: 1px solid rgba(255,255,255,0.05); }"
                            "QStackedWidget#contentStack { background: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.05); }"
                            "QLineEdit#addressOrb { background: %2; border-radius: 18px; font-size: 14px; }"
                            "QLineEdit#addressOrb:focus { border-color: %5; background: #202124; }"
                            "QListWidget#tabDock { background: transparent; border: none; outline: none; }"
                            "QListWidget#tabDock::item { border-radius: 20px; padding: 0; margin: 4px 0; color: %4; }"
                            "QListWidget#tabDock::item:selected { background: %6; border: 1px solid %5; }"
                            "QPushButton#sidebarToggle, QPushButton#settingsBtn, QPushButton#profileBtn { background: transparent; border-radius: 20px; color: %4; text-align: left; padding-left: 10px; }"
                            "QPushButton#sidebarToggle:hover, QPushButton#settingsBtn:hover, QPushButton#profileBtn:hover { background: rgba(255,255,255,0.06); color: %5; }"
                            "QPushButton#newTabBtn { background: transparent; border: 1px solid rgba(255,255,255,0.1); border-radius: 20px; color: %5; font-weight: 600; text-align: left; padding-left: 10px; }"
                            "QPushButton#newTabBtn:hover { background: %6; border-color: %5; }"
                            "QPushButton#navBtn { background: transparent; border-radius: 18px; color: %4; font-size: 14px; border: none; }"
                            "QPushButton#navBtn:hover { background: %6; color: %5; }"
                            "QPushButton#zoomBtn { background: rgba(255,255,255,0.05); border-radius: 6px; color: %4; font-size: 14px; border: none; }"
                            "QPushButton#zoomBtn:hover { background: %6; color: %5; }"
                            "QLineEdit#findBar { background: %2; border: 1px solid rgba(255,255,255,0.08); border-radius: 8px; color: %4; }"
                            ).arg(bg, surf, surf.mid(0,7)==QStringLiteral("rgba(2")?surf:QStringLiteral("#1c2236"), text, accent, a12, sidebar));

    // Update active web views
    for (auto *browser : tabHistoryMap.keys()) applyBrowserPalette(browser);
    setWindowTitle(m_isIncognito ? "GATIWEB — Private" : "GATIWEB Engine");

    // Force Qt to repaint the UI immediately
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}

void MainWindow::applyBrowserPalette(QTextBrowser *b) {
    b->setStyleSheet(QStringLiteral("QTextBrowser { background: %1; color: #e2e8f0; border: none; border-radius: 12px; padding: 14px; }").arg(m_isIncognito ? "#151525" : "#131314"));
}

// --- Settings Dialog ---

void MainWindow::openSettingsDialog() {
    if (m_isIncognito) return; // Hide settings in private mode
    QSettings s("GatiTeam", "GATIWEB");
    QDialog dlg(this); dlg.setWindowTitle("GATIWEB Settings"); dlg.setMinimumWidth(480);
    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
    QTabWidget *tabs = new QTabWidget(&dlg);

    // Tab 1: Homepage setup
    QWidget *homeTab = new QWidget; QVBoxLayout *homeL = new QVBoxLayout(homeTab);
    QString currentHome = s.value("homepage", "https://www.google.com").toString();
    QLineEdit *homeEdit = new QLineEdit(currentHome);

    homeEdit->setFixedHeight(36);
    homeEdit->setPlaceholderText("https://www.google.com");
    homeL->addWidget(new QLabel("Default Core Homepage Target Address:"));
    homeL->addWidget(homeEdit);
    homeL->addStretch();
    tabs->addTab(homeTab, " Homepage ");

    // Tab 2: Theme/Colors
    QWidget *appTab = new QWidget; QVBoxLayout *appL = new QVBoxLayout(appTab);
    QComboBox *presetBox = new QComboBox(&dlg);
    presetBox->addItem("— Custom Accent Hex —", "");
    for (const QString &entry : NAMED_COLORS) presetBox->addItem(entry.split('|')[0], entry.split('|')[1]);

    QString curAccent = s.value("accent_color", "#ff9933").toString();
    QLineEdit *hexEdit = new QLineEdit(curAccent); hexEdit->setMaxLength(7);
    QLabel *swatchPreview = new QLabel(&dlg);
    swatchPreview->setFixedSize(34, 34); swatchPreview->setStyleSheet("background: " + curAccent + "; border-radius: 8px;");

    // Update the color square live as the user picks a theme
    connect(presetBox, &QComboBox::currentIndexChanged, this, [presetBox, hexEdit, swatchPreview](int idx) {
        QString val = presetBox->itemData(idx).toString();
        if (!val.isEmpty()) { hexEdit->setText(val); swatchPreview->setStyleSheet("background: " + val + "; border-radius: 8px;"); }
    });

    appL->addWidget(new QLabel("Select Bharatiya Color Profile:")); appL->addWidget(presetBox);
    QHBoxLayout *hexRow = new QHBoxLayout; hexRow->addWidget(new QLabel("Hex:")); hexRow->addWidget(hexEdit); hexRow->addWidget(swatchPreview);
    appL->addLayout(hexRow); appL->addStretch(); tabs->addTab(appTab, " Appearance ");

    // Tab 3: Security & Adblock
    QWidget *secTab = new QWidget; QVBoxLayout *secL = new QVBoxLayout(secTab);
    QCheckBox *adBlock = new QCheckBox("Activate Core Ad-Block Shield Engine Filters"); adBlock->setChecked(s.value("security/adblock", true).toBool());
    QCheckBox *dnt = new QCheckBox("Transmit Standard 'Do-Not-Track' Header Parameters"); dnt->setChecked(s.value("security/dnt", true).toBool());
    secL->addWidget(adBlock); secL->addWidget(dnt); secL->addStretch(); tabs->addTab(secTab, " Security ");

    dlgLayout->addWidget(tabs);
    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept); connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addWidget(btns);

    if (dlg.exec() != QDialog::Accepted) return;

    // Save and apply settings
    if (QRegularExpression("^#[0-9A-Fa-f]{6}$").match(hexEdit->text()).hasMatch()) {
        m_customAccent = hexEdit->text();
        s.setValue("accent_color", m_customAccent);
    }

    s.setValue("homepage", homeEdit->text().trimmed());
    s.setValue("security/adblock", adBlock->isChecked());
    s.setValue("security/dnt", dnt->isChecked());
    s.sync();

    applyGatiTheme();
}

// --- Tab Management ---

// Create a blank new tab
void MainWindow::addNewTab() {
    QTextBrowser *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(false); browser->setOpenLinks(false);
    applyBrowserPalette(browser);

    // Make clicking links inside the view load via our custom engine
    connect(browser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        addressOrb->setText(QUrl(addressOrb->text()).resolved(url).toString());
        loadUrl();
    });

    tabHistoryMap[browser] = {}; tabHistoryIndexMap[browser] = -1; tabIsSourceViewMap[browser] = false;
    contentStack->setCurrentIndex(contentStack->addWidget(browser));

    // Build the sidebar item for this tab
    QListWidgetItem *item = new QListWidgetItem; QWidget *row = new QWidget(tabDock);
    QHBoxLayout *rl = new QHBoxLayout(row); rl->setContentsMargins(8, 2, 8, 2); rl->setSpacing(8);

    QPushButton *favBtn = new QPushButton(m_isIncognito ? "👻" : "💬", row); favBtn->setObjectName("tabFav"); favBtn->setFixedSize(24, 24); favBtn->setStyleSheet("background: transparent; border: none; font-size: 14px;");
    QLabel *titleLbl = new QLabel("New Session", row); titleLbl->setObjectName("tabTitle"); titleLbl->setStyleSheet("color: #e2e8f0; font-size: 13px; background: transparent;"); titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QPushButton *closeBtn = new QPushButton("✕", row); closeBtn->setObjectName("tabClose"); closeBtn->setFixedSize(20, 20); closeBtn->setCursor(Qt::PointingHandCursor); closeBtn->setStyleSheet("QPushButton { background: transparent; color: #4a5568; font-size: 10px; border-radius: 10px; border: none; } QPushButton:hover { background: #e53e3e; color: #fff; }");

    connect(closeBtn, &QPushButton::clicked, this, [this, browser]() { int i = contentStack->indexOf(browser); if (i != -1) closeTab(i); });

    rl->addWidget(favBtn); rl->addWidget(titleLbl, 1); rl->addWidget(closeBtn); row->setLayout(rl);
    item->setSizeHint(QSize(200, 40));
    titleLbl->setVisible(m_sidebarExpanded); closeBtn->setVisible(m_sidebarExpanded);

    tabDock->addItem(item); tabDock->setItemWidget(item, row);
    tabItemToBrowser[item] = browser; browserToTabItem[browser] = item; tabDock->setCurrentItem(item);

    // Load the default page
    QString home = getHomePageUrl();
    if (!home.isEmpty() && home != "https://www.google.com") {
        // If user set a custom URL other than Google, load it
        addressOrb->setText(home);
        loadUrl();
    } else {
        // Otherwise, render the zero-latency local dashboard
        addressOrb->clear();
        browser->setHtml(getDashboardHtml());
    }
    updateNavigationButtons();
}

// Kill a tab and wipe its data from memory
void MainWindow::closeTab(int index) {
    if (contentStack->count() <= 1) return; // Don't close the last tab
    QWidget *w = contentStack->widget(index);
    QTextBrowser *br = qobject_cast<QTextBrowser*>(w);
    if (!br) return;

    // Save the URL to our 'recently closed' stack before we delete it (max 15 limits RAM usage)
    int hIdx = tabHistoryIndexMap.value(br, -1);
    if (hIdx >= 0 && hIdx < tabHistoryMap[br].size()) {
        m_closedTabs.append(tabHistoryMap[br].at(hIdx));
        if (m_closedTabs.size() > 15) m_closedTabs.removeFirst();
    }

    // Cancel any network requests this tab was currently waiting on
    QList<QNetworkReply*> aborts;
    for (auto it = replyToBrowser.begin(); it != replyToBrowser.end(); ++it) if (it.value() == br) aborts.append(it.key());
    for (auto *r : aborts) { replyToBrowser.remove(r); r->abort(); r->deleteLater(); }

    // Remove UI elements
    QListWidgetItem *ti = browserToTabItem.value(br);
    if (ti) { browserToTabItem.remove(br); tabItemToBrowser.remove(ti); delete tabDock->takeItem(tabDock->row(ti)); }
    tabHistoryMap.remove(br); tabHistoryIndexMap.remove(br); tabIsSourceViewMap.remove(br);

    int ni = (index > 0) ? index - 1 : 0;
    contentStack->removeWidget(w); delete w;

    // Switch focus to the adjacent tab
    if (contentStack->count() > 0) {
        contentStack->setCurrentIndex(ni);
        if (auto *active = qobject_cast<QTextBrowser*>(contentStack->currentWidget())) if (auto *ai = browserToTabItem.value(active)) tabDock->setCurrentItem(ai);
        updateNavigationButtons();
    }
}

// Click on the sidebar to change the active tab
void MainWindow::switchToTab(QListWidgetItem *item) {
    auto *br = tabItemToBrowser.value(item);
    if (!br) return;
    int i = contentStack->indexOf(br);
    if (i != -1) contentStack->setCurrentIndex(i);

    int hi = tabHistoryIndexMap[br];
    addressOrb->setText((hi >= 0 && hi < tabHistoryMap[br].size()) ? tabHistoryMap[br].at(hi) : "");
    updateSecureIcon(addressOrb->text()); updateNavigationButtons();
    setLoadingState(replyToBrowser.values().contains(br));
}

// Resurrect the last closed tab (Ctrl+Shift+T)
void MainWindow::undoClosedTab() {
    if (m_closedTabs.isEmpty()) return;
    QString restoredUrl = m_closedTabs.takeLast();
    addNewTab();
    addressOrb->setText(restoredUrl);
    loadUrl();
}

// --- Network & Routing ---

// Make sure the URL has 'https://' or perform a Google search if it's just words
QString MainWindow::resolveUrl(const QString &raw) const {
    QString u = raw.trimmed(); if (u.isEmpty()) return {};
    if (!u.contains('.') || u.contains(' ')) return "https://www.google.com/search?q=" + QUrl::toPercentEncoding(u);
    if (!u.startsWith("http://") && !u.startsWith("https://")) u.prepend("https://"); return u;
}

bool MainWindow::isHttpsUrl(const QString &url) const { return url.startsWith("https://"); }

void MainWindow::updateSecureIcon(const QString &url) {
    if (url.isEmpty()) { secureIcon->setText(""); return; }
    if (isHttpsUrl(url)) { secureIcon->setText("🔒"); secureIcon->setStyleSheet("QLabel { color: #10b981; }"); }
    else { secureIcon->setText("🔓"); secureIcon->setStyleSheet("QLabel { color: #f59e0b; }"); }
}

// Setup the network packet with spoofed headers and DNT flag
QNetworkRequest MainWindow::buildRequest(const QString &url) const {
    QNetworkRequest req; req.setUrl(QUrl(url)); req.setRawHeader("User-Agent", UA.toLatin1());
    req.setRawHeader("DNT", QSettings("GatiTeam","GATIWEB").value("security/dnt", true).toBool() ? "1" : "0");
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache); // Use local disk cache if available
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}

// Check if domain hits the blocklist filter
bool MainWindow::isAdBlocked(const QString &url) const {
    if (!QSettings("GatiTeam","GATIWEB").value("security/adblock", true).toBool()) return false;
    return AD_BLOCK_REGEX.match(url).hasMatch();
}

// User clicked the 'X' button to halt the download
void MainWindow::stopLoading() {
    auto *browser = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    QList<QNetworkReply*> aborts;
    for (auto it = replyToBrowser.begin(); it != replyToBrowser.end(); ++it) if (it.value() == browser) aborts.append(it.key());
    for (auto *r : aborts) { replyToBrowser.remove(r); r->abort(); r->deleteLater(); }
    setLoadingState(false);
}

// Trigger the actual network fetch process
void MainWindow::loadUrl() {
    QString url = resolveUrl(addressOrb->text());
    if (url.isEmpty()) return;
    addressOrb->setText(url); updateSecureIcon(url);

    auto *browser = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!browser) return;

    // Clear old page data from memory
    browser->setProperty("rawHtml", QVariant());

    // Intercept blocked domains immediately
    if (isAdBlocked(url)) {
        applyBrowserPalette(browser);
        browser->setHtml(QStringLiteral("<html><body style='background:transparent;color:#e2e8f0;text-align:center;padding:60px'><h2 style='color:#f59e0b'>🛡 Blocked</h2><p style='color:#64748b'>This domain is on GATIWEB's shield list.</p></body></html>"));
        return;
    }

    // Save history (capped at 50 to prevent massive memory usage over time)
    int curIdx = tabHistoryIndexMap[browser];
    QStringList hist = tabHistoryMap[browser];
    while (hist.size() > curIdx + 1) hist.removeLast();
    hist.append(url);
    while(hist.size() > 50) hist.removeFirst();
    tabHistoryMap[browser] = hist; tabHistoryIndexMap[browser] = hist.size() - 1;

    // Execute the request
    QNetworkReply *reply = networkManager->get(buildRequest(url));
    replyToBrowser[reply] = browser;
    connect(reply, &QNetworkReply::downloadProgress, this, &MainWindow::handleDownloadProgress);
    if (auto *lbl = tabTitleLabel(browser)) lbl->setText("Loading…");
    setLoadingState(true); updateNavigationButtons();
}

void MainWindow::handleDownloadProgress(qint64 recv, qint64 total) { if (total > 0) loadProgress->setValue(int(recv * 90 / total)); }

// Process the finished webpage data
void MainWindow::handleNetworkData(QNetworkReply *reply) {
    if (!replyToBrowser.contains(reply)) { reply->deleteLater(); return; }
    auto *browser = replyToBrowser.take(reply);
    if (!browser) return;
    if (browser == contentStack->currentWidget()) setLoadingState(false);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll(); QString html = QString::fromUtf8(data);

        // Parse meta-refresh tags to automatically redirect (avoids the "click here" stuck screen)
        // We only scan the first 4096 bytes (the head) so we don't freeze the CPU parsing massive files
        QStringView headHtml = QStringView(html).left(qMin(html.length(), 4096));
        QRegularExpressionMatchIterator it = META_RE.globalMatch(headHtml);

        while (it.hasNext()) {
            QString metaTag = it.next().captured(0);
            if (metaTag.toLower().contains("refresh") && metaTag.toLower().contains("url=")) {
                if (URL_RE.match(metaTag).hasMatch()) {
                    addressOrb->setText(reply->url().resolved(QUrl(URL_RE.match(metaTag).captured(1).replace("&amp;", "&"))).toString());
                    reply->deleteLater(); loadUrl(); return;
                }
            }
        }

        // Render the page
        browser->setProperty("rawHtml", html);
        tabIsSourceViewMap.value(browser, false) ? browser->setPlainText(html) : browser->setHtml(html);
        if (auto *lbl = tabTitleLabel(browser)) lbl->setText(reply->url().host().isEmpty() ? "GATIWEB" : reply->url().host());

        if (browser == contentStack->currentWidget()) {
            // Make sure we don't overwrite the address bar if the user is currently typing in it
            if (!addressOrb->hasFocus()) {
                addressOrb->setText(reply->url().toString());
            }
            updateSecureIcon(reply->url().toString());
        }
    }
    reply->deleteLater();
}

// --- Navigation & Actions ---

void MainWindow::goBack() {
    auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br || tabHistoryIndexMap[br] <= 0) return;
    QString url = tabHistoryMap[br].at(--tabHistoryIndexMap[br]);
    addressOrb->setText(url); updateSecureIcon(url);
    replyToBrowser[networkManager->get(buildRequest(url))] = br;
    setLoadingState(true); updateNavigationButtons();
}

void MainWindow::goForward() {
    auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br || tabHistoryIndexMap[br] >= tabHistoryMap[br].size() - 1) return;
    QString url = tabHistoryMap[br].at(++tabHistoryIndexMap[br]);
    addressOrb->setText(url); updateSecureIcon(url);
    replyToBrowser[networkManager->get(buildRequest(url))] = br;
    setLoadingState(true); updateNavigationButtons();
}

void MainWindow::refreshPage() {
    auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br) return;
    if (tabHistoryIndexMap[br] < 0) { QString fb = addressOrb->text().trimmed(); if (fb.isEmpty()) fb = getHomePageUrl(); if (!fb.isEmpty()) { addressOrb->setText(fb); loadUrl(); } return; }
    replyToBrowser[networkManager->get(buildRequest(tabHistoryMap[br].at(tabHistoryIndexMap[br])))] = br;
    setLoadingState(true);
}

void MainWindow::zoomIn() {
    m_zoomFactor = qMin(m_zoomFactor + 0.1, 3.0);
    if (auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget())) { QFont f = br->font(); f.setPointSizeF(f.pointSizeF() * 1.1); br->setFont(f); }
    zoomLabel->setText(QString("%1%").arg(int(m_zoomFactor * 100)));
}

void MainWindow::zoomOut() {
    m_zoomFactor = qMax(m_zoomFactor - 0.1, 0.3);
    if (auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget())) { QFont f = br->font(); f.setPointSizeF(f.pointSizeF() / 1.1); br->setFont(f); }
    zoomLabel->setText(QString("%1%").arg(int(m_zoomFactor * 100)));
}

void MainWindow::zoomReset() {
    m_zoomFactor = 1.0;
    if (auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget())) br->setFont(QFont());
    zoomLabel->setText("100%");
}

// Toggle between rendered HTML and raw source code
void MainWindow::viewPageSource() {
    auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br || br->property("rawHtml").toString().isEmpty()) return;
    tabIsSourceViewMap[br] = !tabIsSourceViewMap.value(br, false);
    if (tabIsSourceViewMap[br]) { br->setPlainText(br->property("rawHtml").toString()); sourceButton->setText("🌐"); }
    else { br->setHtml(br->property("rawHtml").toString()); sourceButton->setText("{ }"); }
}

void MainWindow::saveBookmark() {
    if (m_isIncognito) return;
    QString url = addressOrb->text().trimmed(); if (url.isEmpty()) return;
    QSettings s("GatiTeam","GATIWEB"); QStringList bm = s.value("bookmarks").toStringList();
    if (!bm.contains(url)) { bm.append(url); s.setValue("bookmarks", bm); }
    updateBookmarkMenu();
}

void MainWindow::updateBookmarkMenu() {
    bookmarkMenu->clear();
    connect(bookmarkMenu->addAction("➕ Save current page"), &QAction::triggered, this, &MainWindow::saveBookmark);
    bookmarkMenu->addSeparator();
    const QStringList bms = QSettings("GatiTeam","GATIWEB").value("bookmarks").toStringList();
    for (const QString &url : bms) connect(bookmarkMenu->addAction(url), &QAction::triggered, this, [this, url]() { addressOrb->setText(url); loadUrl(); });
    if (!bms.isEmpty()) {
        bookmarkMenu->addSeparator();
        connect(bookmarkMenu->addAction("🗑 Clear all"), &QAction::triggered, this, [this]() {
            if (QMessageBox::question(this, "Clear", "Delete all bookmarks?") == QMessageBox::Yes) { QSettings("GatiTeam","GATIWEB").remove("bookmarks"); updateBookmarkMenu(); }
        });
    }
}

// Highlight text on the page
void MainWindow::findText() {
    QString q = findBar->text(); auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br || q.isEmpty()) { findResultLabel->setText(""); return; }
    br->setFocus(); bool found = br->find(q);
    if (!found) { QTextCursor c = br->textCursor(); c.movePosition(QTextCursor::Start); br->setTextCursor(c); found = br->find(q); }
    findResultLabel->setText(found ? "✓" : "✗");
}

void MainWindow::showFindBar() { findBar->setVisible(true); findCloseButton->setVisible(true); findResultLabel->setVisible(true); findBar->setFocus(); findBar->selectAll(); }

void MainWindow::updateNavigationButtons() {
    auto *br = qobject_cast<QTextBrowser*>(contentStack->currentWidget());
    if (!br) { backButton->setEnabled(false); forwardButton->setEnabled(false); return; }
    backButton->setEnabled(tabHistoryIndexMap.value(br, -1) > 0);
    forwardButton->setEnabled(tabHistoryIndexMap.value(br, -1) < tabHistoryMap.value(br).size() - 1);
}

void MainWindow::openIncognitoWindow() {
    if (m_isIncognito) { QMessageBox::information(this,"Private","You're already private."); return; }
    auto *w = new MainWindow(true); w->resize(size()); w->show();
}

QString MainWindow::getHomePageUrl() const { return m_isIncognito ? QString() : QSettings("GatiTeam","GATIWEB").value("homepage", "https://www.google.com").toString(); }
QLabel* MainWindow::tabTitleLabel(QTextBrowser *br) { return browserToTabItem.contains(br) ? tabDock->itemWidget(browserToTabItem[br])->findChild<QLabel*>("tabTitle") : nullptr; }
QPushButton* MainWindow::tabIconLabel(QTextBrowser *br) { return browserToTabItem.contains(br) ? tabDock->itemWidget(browserToTabItem[br])->findChild<QPushButton*>("tabFav") : nullptr; }

// ============================================================================
//  SECTION 9: INTERNAL HTML DASHBOARD RENDERING
//  Purpose: Generates a zero-latency, hardcoded local start page.
// ============================================================================
QString MainWindow::getDashboardHtml() const {
    const QString accent = m_isIncognito ? "#f43f5e" : m_customAccent;

    return QStringLiteral(R"(
        <html><head><style>
        body { background: transparent; color: #e2e8f0; font-family: system-ui, sans-serif; margin: 0; padding: 50px 20px; display: flex; flex-direction: column; align-items: center; }
        .hero { text-align: center; margin-bottom: 40px; }
        h1 { font-size: 3.5em; color: %1; margin: 0 0 10px 0; letter-spacing: 2px; }
        p.subtitle { color: #64748b; font-size: 1.1em; margin: 0; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; width: 100%; max-width: 900px; }
        .card { background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.05); border-radius: 12px; padding: 20px; }
        h3 { color: #f8f9fa; margin: 0 0 15px 0; font-size: 1.2em; border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 8px; }
        a { display: block; color: #94a3b8; text-decoration: none; padding: 8px 0; font-size: 14px; font-weight: 500; }
        </style></head><body>

        <div class='hero'>
            <h1>GATIWEB</h1>
            <p class='subtitle'>High-Performance Native Navigation Core</p>
        </div>

        <div class='grid'>
            <div class='card'>
                <h3>📰 Lite Media Engine</h3>
                <a href='https://lite.cnn.com'>CNN Lite (Global News)</a>
                <a href='https://text.npr.org'>NPR Text (Audio/News)</a>
                <a href='http://68k.news'>68k News (Aggregator)</a>
            </div>
            <div class='card'>
                <h3>💻 Tech & Systems</h3>
                <a href='https://news.ycombinator.com'>Hacker News</a>
                <a href='https://lobste.rs'>Lobsters (Tech Forums)</a>
                <a href='https://lite.duckduckgo.com/lite/'>DuckDuckGo Lite (Search)</a>
            </div>
            <div class='card'>
                <h3>⚡ Data Utilities</h3>
                <a href='https://wttr.in'>wttr.in (ASCII Weather)</a>
                <a href='https://plaintextsports.com'>Plain Text Sports (Live)</a>
                <a href='https://www.gutenberg.org'>Project Gutenberg (Books)</a>
            </div>
        </div>

        </body></html>
    )").arg(accent);
}