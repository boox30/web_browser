#include "url_lineedit.h"
#include "view.h"
#include "web_application.h"
#include "common/WindowsMetrics.h"
#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QCompleter>
#include <QtGui/QFocusEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionFrameV2>
using namespace windowsmetrics;

namespace  webbrowser
{

static const QString READER_BUTTON_STYLE = "                          \
QAbstractButton                             \
{                                       \
    background: dark;                   \
    border: 1px                         \
    border-width: 1px;                  \
    border-color: black;                \
    border-style: solid;                \
    color: black;                       \
    padding: 2px;                       \
}                                       \
QAbstractButton:pressed                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    color: white;                       \
    border-color: black;                \
    background-color: black;            \
}                                       \
QAbstractButton:checked                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    color: white;                       \
    border-color: black;                \
    background-color: black;            \
}                                       \
QAbstractButton:focus                       \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border: 1px                         \
    color: black;                       \
    border-width: 0px;                  \
    border-color: black;                \
    background-color: dark;            \
}                                       \
QAbstractButton:disabled                    \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: dark;                 \
    color: dark;                        \
    background-color: white;            \
}";


ClearButton::ClearButton(QWidget* parent)
    : QAbstractButton(parent)
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif
    setVisible(false);
    setFocusPolicy(Qt::NoFocus);
}

void ClearButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    int height = this->height();

    painter.setRenderHint(QPainter::Antialiasing, true);
    //QColor color = palette().color(QPalette::Mid);
    painter.setBrush(isDown()
                     ? palette().color(QPalette::Dark)
                     : palette().color(QPalette::Mid));
    painter.setPen(painter.brush().color());
    int size = width();
    int offset = size / 5;
    int radius = size - offset * 2;
    painter.drawEllipse(offset, offset, radius, radius);

    painter.setPen(Qt::black);
    int border = offset * 2;
    painter.drawLine(border, border, width() - border, height - border);
    painter.drawLine(border, height - border, width() - border, border);
}

void ClearButton::textChanged(const QString &text)
{
    setVisible(!text.isEmpty());
}

// Reader Button
ReaderButton::ReaderButton(QWidget* parent)
    : QAbstractButton(parent)
    , is_reader_mode_(false)
{
    setText(QApplication::tr("reader"));
    setMyStyleSheet();
}

void ReaderButton::setMyStyleSheet()
{
    setStyleSheet(READER_BUTTON_STYLE);
}

void ReaderButton::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    QFont btnFont = font();
    btnFont.setBold(true);
    btnFont.setPixelSize(ui::windowsmetrics::GetWindowFontSize(ui::windowsmetrics::DKPushButtonIndex));
    setFont(btnFont);
    if (pressed_)
    {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, text());
    }
    else
    {
        painter.setPen(Qt::black);
        painter.drawText(rect(), Qt::AlignCenter, text());
    }
}

void ReaderButton::mousePressEvent(QMouseEvent* e)
{
    pressed_ = true;
    QAbstractButton::mousePressEvent(e);
}

void ReaderButton::mouseReleaseEvent(QMouseEvent* e)
{
    pressed_ = false;
    QAbstractButton::mouseReleaseEvent(e);
}

void ReaderButton::setReaderMode(bool is_reader_mode)
{
    if (is_reader_mode_ == is_reader_mode)
    {
        return;
    }

    is_reader_mode_ = is_reader_mode;
    if (is_reader_mode_)
    {
        setText(QApplication::tr("normal"));
    }
    else
    {
        setText(QApplication::tr("reader"));
    }
    update();
}

ExLineEdit::ExLineEdit(QWidget *parent)
    : QWidget(parent)
    , left_widget_(0)
    , line_edit_(new DKLineEdit(this))
    , reader_button_(0)
    , clear_button_(0)
    , modify_line_edit_text_automatically_(true)
{
    setFocusPolicy(line_edit_->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
    setSizePolicy(line_edit_->sizePolicy());
    setBackgroundRole(line_edit_->backgroundRole());
    setMouseTracking(true);
    //setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect, true);
    QPalette p = line_edit_->palette();
    setPalette(p);

    // line edit
    line_edit_->setFrame(false);
    line_edit_->setFocusProxy(this);
    line_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
    QPalette clearPalette = line_edit_->palette();
    clearPalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    line_edit_->setPalette(clearPalette);

    // clearButton
    reader_button_ = new ReaderButton(0);
    connect(reader_button_, SIGNAL(clicked()),
            this, SLOT(onReaderButtonClicked()));

    // clearButton
    clear_button_ = new ClearButton(this);
    connect(clear_button_, SIGNAL(clicked()),
            line_edit_, SLOT(clear()));
    connect(line_edit_, SIGNAL(textChanged(QString)),
            clear_button_, SLOT(textChanged(QString)));
    setStyleSheet("border: 1px solid dark");
}

void ExLineEdit::setLeftWidget(QWidget *widget)
{
    left_widget_ = widget;
}

QWidget *ExLineEdit::leftWidget() const
{
    return left_widget_;
}

void ExLineEdit::resizeEvent(QResizeEvent *event)
{
    Q_ASSERT(left_widget_);
    updateGeometries();
    QWidget::resizeEvent(event);
}

void ExLineEdit::updateGeometries()
{
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

    int height = rect.height();
    int width = rect.width();

    int m_leftWidgetHeight = left_widget_->height();
    left_widget_->setGeometry(rect.x() + 2,          rect.y() + (height - m_leftWidgetHeight)/2,
                              left_widget_->width(), left_widget_->height());

    int clearButtonWidth = this->height();
    line_edit_->setGeometry(left_widget_->x() + left_widget_->width() + 2,        1,
                            width - clearButtonWidth - left_widget_->width(), this->height() - 2);

    clear_button_->setGeometry(this->width() - clearButtonWidth, 0,
                               clearButtonWidth, this->height());

    int readerButtonWidth = reader_button_->width();
    reader_button_->setGeometry(this->width() - clearButtonWidth - readerButtonWidth - 4, 0,
                               readerButtonWidth, this->height());
}

void ExLineEdit::initStyleOption(QStyleOptionFrameV2 *option) const
{
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this);
    option->midLineWidth = 0;
    //option->state |= QStyle::State_Sunken;
    if (line_edit_->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrameV2::None;
}

QSize ExLineEdit::sizeHint() const
{
    line_edit_->setFrame(true);
    QSize size = line_edit_->sizeHint();
    line_edit_->setFrame(false);
    return size;
}

void ExLineEdit::focusInEvent(QFocusEvent *event)
{
    line_edit_->event(event);
    QWidget::focusInEvent(event);
}

void ExLineEdit::focusOutEvent(QFocusEvent *event)
{
    line_edit_->event(event);

    if (line_edit_->completer()) {
        connect(line_edit_->completer(), SIGNAL(activated(QString)),
                         line_edit_, SLOT(setText(QString)));
        connect(line_edit_->completer(), SIGNAL(highlighted(QString)),
                         line_edit_, SLOT(_q_completionHighlighted(QString)));
    }
    QWidget::focusOutEvent(event);
}

void ExLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
    {
        event->ignore();
    }
    else
    {
        line_edit_->event(event);
    }
}

bool ExLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride)
        return line_edit_->event(event);
    return QWidget::event(event);
}

void ExLineEdit::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
}

QVariant ExLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return line_edit_->inputMethodQuery(property);
}

void ExLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    line_edit_->event(e);
}

void ExLineEdit::onReaderButtonClicked()
{
    bool is_reader_mode = !reader_button_->isReaderMode();
    reader_button_->setReaderMode(is_reader_mode);
    emit enterReaderMode(is_reader_mode);
}

void ExLineEdit::displayReaderButton(bool display)
{
    if (reader_button_->isVisible() == display)
    {
        return;
    }
    if (!reader_button_->isVisible())
    {
        reader_button_->setReaderMode(false);
    }
    reader_button_->setVisible(display);
}

class UrlIconLabel : public QLabel
{

public:
    UrlIconLabel(QWidget *parent);

    BrowserView *web_view_;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint m_dragStartPos;

};

UrlIconLabel::UrlIconLabel(QWidget *parent)
    : QLabel(parent)
    , web_view_(0)
{
    setStyleSheet("border: 0px solid dark");
    setAlignment(Qt::AlignCenter);
    setMinimumWidth(GetWindowMetrics(UrlFaviconSizeIndex));
    setMinimumHeight(GetWindowMetrics(UrlFaviconSizeIndex));
}

void UrlIconLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QLabel::mousePressEvent(event);
}

void UrlIconLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton
        && (event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
         && web_view_) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString::fromUtf8(web_view_->url().toEncoded()));
        QList<QUrl> urls;
        urls.append(web_view_->url());
        mimeData->setUrls(urls);
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

UrlLineEdit::UrlLineEdit(QWidget *parent)
    : ExLineEdit(parent)
    , web_view_(0)
    , icon_label_(0)
{
    // icon
    icon_label_ = new UrlIconLabel(this);
    icon_label_->resize(GetWindowMetrics(UrlFaviconSizeIndex), GetWindowMetrics(UrlFaviconSizeIndex));
    setLeftWidget(icon_label_);
    default_base_color_ = palette().color(QPalette::Base);

    webViewIconChanged();
}

void UrlLineEdit::setWebView(BrowserView *webView)
{
    Q_ASSERT(!web_view_);
    web_view_ = webView;
    icon_label_->web_view_ = webView;
    connect(webView, SIGNAL(urlChanged(QUrl)),
        this, SLOT(webViewUrlChanged(QUrl)));
    connect(webView, SIGNAL(loadFinished(bool)),
        this, SLOT(webViewIconChanged()));
    connect(webView, SIGNAL(iconChanged()),
        this, SLOT(webViewIconChanged()));
    connect(webView, SIGNAL(progressChangedSignal(const int, const int)),
        this, SLOT(update()));
}

void UrlLineEdit::webViewUrlChanged(const QUrl &url)
{
    if (modify_line_edit_text_automatically_)
    {
        line_edit_->setText(QString::fromUtf8(url.toEncoded()));
        line_edit_->setCursorPosition(0);
    }
}

void UrlLineEdit::webViewIconChanged()
{
    QUrl url = (web_view_)  ? web_view_->url() : QUrl();
    QIcon icon = WebApplication::instance()->icon(url);
    QPixmap pixmap(icon.pixmap(GetWindowMetrics(UrlFaviconSizeIndex), GetWindowMetrics(UrlFaviconSizeIndex)));
    icon_label_->setPixmap(pixmap);
}

QLinearGradient UrlLineEdit::generateGradient(const QColor &color) const
{
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, default_base_color_);
    gradient.setColorAt(0.15, color.lighter(120));
    gradient.setColorAt(0.5, color);
    gradient.setColorAt(0.85, color.lighter(120));
    gradient.setColorAt(1, default_base_color_);
    return gradient;
}

void UrlLineEdit::focusOutEvent(QFocusEvent *event)
{
    if (line_edit_->text().isEmpty() && web_view_ && modify_line_edit_text_automatically_)
        line_edit_->setText(QString::fromUtf8(web_view_->url().toEncoded()));
    ExLineEdit::focusOutEvent(event);
}

void UrlLineEdit::paintEvent(QPaintEvent *event)
{
    QPalette p = palette();
    if (web_view_ && web_view_->url().scheme() == QLatin1String("https")) {
        QColor lightYellow(248, 248, 210);
        p.setBrush(QPalette::Base, generateGradient(lightYellow));
    } else {
        p.setBrush(QPalette::Base, default_base_color_);
    }
    setPalette(p);
    ExLineEdit::paintEvent(event);

    QPainter painter(this);
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    QRect backgroundRect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    if (web_view_ && !hasFocus()) {
        int progress = web_view_->getProgress();
        QColor loadingColor = QColor(116, 192, 250);
        //painter.setBrush(generateGradient(loadingColor));
        painter.setBrush(loadingColor);
        painter.setPen(loadingColor);
        int mid = backgroundRect.width() / 100.0 * progress;
        QRect progressRect(backgroundRect.x(), backgroundRect.y() + 2, mid, backgroundRect.height() - 4);
        painter.drawRect(progressRect);
    }
    
}
}//webbrowser
