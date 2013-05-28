#include <QtGui/QtGui>
#include "browser_mainwindow.h"
#include "web_application.h"
#include "Screen/screen_proxy.h"
#include "Screen/screen_update_watcher.h"

namespace webbrowser
{
BrowserMainWindow::BrowserMainWindow(QWidget *parent)
#ifdef Q_WS_QWS
    : QMainWindow(parent, Qt::FramelessWindowHint)
#else
    : QMainWindow(parent)
#endif
    , homepage_action_(QIcon(QLatin1String(":/res/browser@kt.png")), "", this)
    , history_back_action_(QIcon(QLatin1String(":/res/back@kt.png")), "", this)
    , history_forward_action_(QIcon(QLatin1String(":/res/forward@kt.png")), "", this)
    , menu_action(QIcon(QLatin1String(":/res/menu@kt.png")), "", this)
    , address_lineedit(this)
    //, layout_(this)
    , view_(NULL)
    , keyboard_status_(KEYBOARD_FREE)
{
    // setAttribute(Qt::WA_DeleteOnClose, true);
#ifndef Q_WS_QWS
    resize(600, 800);
#else
    resize(qApp->desktop()->screenGeometry().size());
#endif
    setupToolBar();

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    setCentralWidget(&view_);
    //createLayout();

    connect(&view_, SIGNAL(progressChangedSignal(const int, const int)),
            this, SLOT(onProgressChanged(const int, const int)));
    connect(&view_, SIGNAL(showHome()), this, SLOT(showHomePage()));
    connect(&view_, SIGNAL(connectionChanged(WifiProfile&, WpaConnection::ConnectionState)),
            this, SLOT(onConnectionChanged(WifiProfile&, WpaConnection::ConnectionState)));
    connect(&view_, SIGNAL(viewportRangeChangedSignal(const int, const int, const int)),
            this, SLOT(onRangeChanged(const int, const int, const int)));

    connect(&view_, SIGNAL(inputFormFocused(const QString&, const QString&,
                                            const QString&, const QString&,
                                            const QString&, const QString&)),
            this, SLOT(onInputFormFocused(const QString&, const QString&,
                                          const QString&, const QString&,
                                          const QString&, const QString&)));
    connect(&view_, SIGNAL(inputFormLostFocus()), &keyboard_, SLOT(hide()));
    connect(&view_, SIGNAL(focusOut()), this, SLOT(onWebViewFocusOut()));

    // Keyboard
    connect(&keyboard_, SIGNAL(textFinsihed(const QString&)), this, SLOT(onTextFinished(const QString&)));

#ifdef Q_WS_QWS
    connect(qApp->desktop(), SIGNAL(resized(int)),
            this, SLOT(onScreenSizeChanged(int)), Qt::QueuedConnection);
#endif
}

BrowserMainWindow::~BrowserMainWindow()
{
}

static QUrl guessUrlFromString(const QString &string)
{
    QString url_str = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // Check if it looks like a qualified URL. Try parsing it and see.
    bool has_schema = test.exactMatch(url_str);
    if (has_schema)
    {
        QUrl url = QUrl::fromEncoded(url_str.toUtf8(), QUrl::TolerantMode);
        if (url.isValid())
        {
            return url;
        }
    }

    // Might be a file.
    if (QFile::exists(url_str))
    {
        QFileInfo info(url_str);
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }

    // Might be a shorturl - try to detect the schema.
    if (!has_schema)
    {
        int dot_index = url_str.indexOf(QLatin1Char('.'));
        if (dot_index != -1)
        {
            QString prefix = url_str.left(dot_index).toLower();
            QByteArray schema = (prefix == QLatin1String("ftp")) ? prefix.toLatin1() : "http";
            QUrl url = QUrl::fromEncoded(schema + "://" + url_str.toUtf8(), QUrl::TolerantMode);
            if (url.isValid())
            {
                return url;
            }
        }
    }

    // Fall back to QUrl's own tolerant parser.
    QUrl url = QUrl::fromEncoded(string.toUtf8(), QUrl::TolerantMode);

    // finally for cases where the user just types in a hostname add http
    if (url.scheme().isEmpty())
    {
        url = QUrl::fromEncoded("http://" + string.toUtf8(), QUrl::TolerantMode);
    }
    return url;
}

void BrowserMainWindow::load(const QString & url_str)
{
    // Depends on the url is local file or not, if the url is empty
    // we need to display the thumbnail view.
    // Check if it's a local file or not at first.
    QUrl url = guessUrlFromString(url_str);
    if (!url.isValid())
    {
    }
    else
    {
        QString host = url.host();
        if (!host.isEmpty())
        {
            QString message = tr("%1");
            message = message.arg(host);
        }

        if (xiaomi_account_manager_.isXiaomiAccountPath(url_str))
        {
            xiaomi_account_manager_.connectWebView(&view_);
        }

        view_.myLoad(url);
    }
}

bool BrowserMainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip ||
        e->type() == QEvent::HoverMove ||
        e->type() == QEvent::HoverEnter ||
        e->type() == QEvent::HoverLeave)
    {
        e->accept();
        return true;
    }

    bool ret = QWidget::event(e);
    return ret;
}

void BrowserMainWindow::setupToolBar()
{
    QToolBar* navigationBar = addToolBar(tr("Navigation"));
    navigationBar->addAction(&homepage_action_);
    navigationBar->addAction(&history_back_action_);
    navigationBar->addAction(&history_forward_action_);
    navigationBar->addWidget(&address_lineedit);
    navigationBar->addAction(&menu_action);
}

//void BrowserMainWindow::createLayout()
//{
    //layout_.setContentsMargins(0, 0, 0, 0);
    //layout_.setSpacing(1);
    //layout_.addWidget(&view_);
    //layout_.addWidget(&keyboard_);
    //keyboard_.setVisible(false);
//}

void BrowserMainWindow::keyReleaseEvent(QKeyEvent *ke)
{
    ke->accept();
    switch (ke->key())
    {
    case Qt::Key_Escape:
        view_.returnToLibrary();
        break;
    default:
        QWidget::keyReleaseEvent(ke);
        break;
    }
}

/// The keyPressEvent could be sent from virtual keyboard.
void BrowserMainWindow::keyPressEvent(QKeyEvent * ke)
{
    ke->accept();

    QKeyEvent * key_event = new QKeyEvent(ke->type(), ke->key(), ke->modifiers(), ke->text());
    QApplication::postEvent(&view_, key_event);

    while (QApplication::hasPendingEvents())
    {
        QApplication::processEvents();
    }
}

void BrowserMainWindow::closeEvent(QCloseEvent *e)
{
    QWidget::closeEvent(e);
}

void BrowserMainWindow::onScreenSizeChanged(int)
{
    resize(qApp->desktop()->screenGeometry().size());
}

void BrowserMainWindow::onInputFormFocused(const QString& form_id,
                                      const QString& form_name,
                                      const QString& form_action,
                                      const QString& input_type,
                                      const QString& input_id,
                                      const QString& input_name)
{
    // fill keyboard private data
    keyboard_priv_.form_action = form_action;
    keyboard_priv_.form_id     = form_id;
    keyboard_priv_.form_name   = form_name;
    keyboard_priv_.input_type  = input_type;
    keyboard_priv_.input_id    = input_id;
    keyboard_priv_.input_name  = input_name;

    // update keyboard status
    keyboard_status_ = FORM_FOCUSED;
    keyboard_.setVisible(true);
}

void BrowserMainWindow::onTextFinished(const QString& text)
{
    // reset keyboard status
    if (keyboard_status_ == FORM_FOCUSED)
    {
        view_.formFocusedAddValue(keyboard_priv_.form_id,
                                  keyboard_priv_.form_name,
                                  keyboard_priv_.form_action,
                                  keyboard_priv_.input_type,
                                  keyboard_priv_.input_id,
                                  keyboard_priv_.input_name,
                                  text);
    }

    // refresh screen
    keyboard_.hide();
    keyboard_.clearText();

    if (keyboard_status_ == URL_INPUTTING)
    {
        load(text);
    }
    keyboard_status_ = KEYBOARD_FREE;
}

void BrowserMainWindow::onInputUrl()
{
    if (keyboard_status_ != URL_INPUTTING || keyboard_.isHidden())
    {
        // update keyboard status
        keyboard_status_ = URL_INPUTTING;
        keyboard_.setVisible(true);
    }
    else if (keyboard_.isVisible())
    {
        keyboard_.setVisible(false);
        keyboard_status_ = KEYBOARD_FREE;
    }
}

void BrowserMainWindow::onInputText()
{
}

}