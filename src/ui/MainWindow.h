#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialog>
#include <QtWidgets/QProgressBar>
#include <QtGui/QShortcut>
#include <QtCore/QSettings>
#include <QtCore/QMap>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkDiskCache> // OPTIMIZATION: Disk Caching Header

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(bool isIncognito = false, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadUrl();
    void goBack();
    void goForward();
    void refreshPage();
    void stopLoading();
    void addNewTab();
    void closeTab(int index);
    void saveBookmark();
    void updateBookmarkMenu();
    void findText();
    void handleNetworkData(QNetworkReply *reply);
    void handleDownloadProgress(qint64 received, qint64 total);
    void viewPageSource();
    void openSettingsDialog();
    void openIncognitoWindow();
    void switchToTab(QListWidgetItem *item);
    void showFindBar();
    void toggleSidebar();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void undoClosedTab();

private:
    bool    m_isIncognito;
    QString m_customAccent;
    bool    m_sidebarExpanded;
    double  m_zoomFactor;

    // UI Components
    QWidget     *leftPanel;
    QPushButton *sidebarToggleButton;
    QListWidget *tabDock;
    QPushButton *newTabButton;
    QPushButton *settingsButton;
    QPushButton *profileButton;

    QPushButton   *backButton;
    QPushButton   *forwardButton;
    QPushButton   *refreshButton;
    QLineEdit     *addressOrb;
    QLabel        *secureIcon;
    QPushButton   *bookmarkButton;
    QPushButton   *sourceButton;
    QPushButton   *incognitoButton;
    QPushButton   *zoomOutBtn;
    QPushButton   *zoomInBtn;
    QLabel        *zoomLabel;
    QMenu         *bookmarkMenu;

    QStackedWidget *contentStack;
    QProgressBar   *loadProgress;

    QLineEdit   *findBar;
    QPushButton *findCloseButton;
    QLabel      *findResultLabel;

    QNetworkAccessManager *networkManager;
    QNetworkCookieJar     *cookieJar;
    bool m_loading = false;

    QMap<QTextBrowser*, QStringList>       tabHistoryMap;
    QMap<QTextBrowser*, int>               tabHistoryIndexMap;
    QMap<QTextBrowser*, bool>              tabIsSourceViewMap;
    QMap<QListWidgetItem*, QTextBrowser*>  tabItemToBrowser;
    QMap<QTextBrowser*, QListWidgetItem*>  browserToTabItem;
    QMap<QNetworkReply*, QTextBrowser*>    replyToBrowser;
    QList<QString> m_closedTabs;

    void    updateNavigationButtons();
    void    applyGatiTheme();
    void    applyBrowserPalette(QTextBrowser *browser);
    void    setLoadingState(bool loading);
    QString resolveUrl(const QString &input) const;
    QString getHomePageUrl() const;
    QString getDashboardHtml() const;
    QString getCustomAccentColor() const;
    bool    isAdBlocked(const QString &url) const;
    bool    isHttpsUrl(const QString &url) const;
    void    updateSecureIcon(const QString &url);
    QNetworkRequest buildRequest(const QString &url) const;
    QLabel* tabTitleLabel(QTextBrowser *browser);
    QPushButton* tabIconLabel(QTextBrowser *browser);
};

#endif // MAINWINDOW_H