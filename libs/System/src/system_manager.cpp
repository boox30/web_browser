#include "system_manager.h"
#ifdef BUILD_FOR_ARM
#include <QtGui/qwsdisplay_qws.h>
#include <QWSPointerCalibrationData>
#include <qwindowsystem_qws.h>
#include <QProxyScreen>
#include <unistd.h>
#endif

#include "KindleTS/kindlets.h"
#include "KindleFiveWay/kindlefiveway.h"
#include "KindleKeyboard/kindlekeyboard.h"
#include "QKindleFb/qkindlefb.h"
#include "Device/device.h"
#include "Gesture/scale_gesture_detector.h"
#include <iostream>
#include <QRegExp>

using namespace gesture;

SystemManager::SystemManager()
    : kindle_ts_(0)
    , kindle_keyboard_(0)
    , kindle_fiveway_(0)
{
    loadDrivers();
}

SystemManager::~SystemManager()
{
    releaseDrivers();
}

SystemManager* SystemManager::instance()
{
    static SystemManager system_manager;
    return &system_manager;
}

void SystemManager::loadDrivers()
{
    releaseDrivers();
    Device& device = Device::instance();
    Device::MODEL model = device.getModel();
    qDebug("Device Model:%d", model);
    switch (model)
    {
    case Device::KPW:
    case Device::KT:
        kindle_ts_ = new KindleTS("kindlets", "debug");
        setupKindleTSMouseHandler();
        break;
    default:
        kindle_keyboard_ = new KindleKeyboard("kindlekeyboard", "debug");
        kindle_fiveway_ = new KindleFiveWay("kindlefiveway", "debug");
        setupKindleKeyboardHandler();
        setupKindleFiveWayHandler();
        break;
    }
}

void SystemManager::releaseDrivers()
{
    if (kindle_ts_ != 0) delete kindle_ts_; kindle_ts_ = 0;
    if (kindle_keyboard_ != 0) delete kindle_keyboard_; kindle_keyboard_ = 0;
    if (kindle_fiveway_ != 0) delete kindle_fiveway_; kindle_fiveway_ = 0;
}

void SystemManager::setupKindleTSMouseHandler()
{
#ifdef BUILD_FOR_ARM
    QWSServer::setMouseHandler(kindle_ts_);
    qDebug("QWSServer::setMouseHandler(kindle_ts_) is Done");
#endif
    ScaleGestureDetector* detector = ScaleGestureDetector::instance();
    connect(kindle_ts_, SIGNAL(touchEvent(TouchEvent*)), detector, SLOT(onTouchEvent(TouchEvent*)));
    qDebug("ScaleGestureDetector::instance() is Done");
}

void SystemManager::setupKindleKeyboardHandler()
{
    qDebug("SystemManager::setupKindleKeyboardHandler");
#ifdef BUILD_FOR_ARM
    QWSServer::setKeyboardHandler(kindle_keyboard_);
#endif
}

void SystemManager::setupKindleFiveWayHandler()
{
    qDebug("SystemManager::setupKindleFiveWayHandler");
#ifdef BUILD_FOR_ARM
    QWSServer::setMouseHandler(kindle_fiveway_);
#endif
}

void SystemManager::suspendTouch()
{
#ifdef BUILD_FOR_ARM
    QWSMouseHandler* mouse_handler = QWSServer::mouseHandler();
    if (mouse_handler != 0)
    {
        mouse_handler->suspend();
    }
#endif
}

void SystemManager::resumeTouch()
{
#ifdef BUILD_FOR_ARM
    QWSMouseHandler* mouse_handler = QWSServer::mouseHandler();
    if (mouse_handler != 0)
    {
        mouse_handler->resume();
    }
#endif
}

// NOTE: this function should only effect on non-touch version
void SystemManager::enterMouseMode()
{
#ifdef BUILD_FOR_ARM
    QWSMouseHandler* mouse_handler = QWSServer::mouseHandler();
    if (mouse_handler != kindle_fiveway_)
    {
        // NOTE: this function should only effect on non-touch version
        qDebug("enterMouseMode can only effect on non-touch version");
        return;
    }
#endif

    kindle_fiveway_->setKeypadMode(false);
}

void SystemManager::enterKeypadMode()
{
#ifdef BUILD_FOR_ARM
    QWSMouseHandler* mouse_handler = QWSServer::mouseHandler();
    if (mouse_handler != kindle_fiveway_)
    {
        // NOTE: this function should only effect on non-touch version
        qDebug("enterKeypadMode can only effect on non-touch version");
        return;
    }
#endif
    kindle_fiveway_->setKeypadMode(true);
}

static QKindleFb* findKindleFb()
{
    QKindleFb* kindle_fb = 0;
#ifdef BUILD_FOR_ARM
    QScreen::ClassId class_id = QScreen::instance()->classId();
    if (class_id == QScreen::TransformedClass ||
        class_id == QScreen::VNCClass ||
        class_id == QScreen::ProxyClass)
    {
        QProxyScreen* proxy = dynamic_cast<QProxyScreen*>(QScreen::instance());
        kindle_fb = dynamic_cast<QKindleFb*>(proxy->screen());
    }
    else
    {
        kindle_fb = dynamic_cast<QKindleFb*>(QScreen::instance());
    }
#endif
    return kindle_fb;
}

void SystemManager::setFullUpdateEvery(int n)
{
    QKindleFb* kindle_fb = findKindleFb();
#ifdef BUILD_FOR_ARM
    if (kindle_fb != 0)
    {
        kindle_fb->setFullUpdateEvery(n);
    }
#endif
}

void SystemManager::forceFullUpdate(bool fullScreen)
{
    QKindleFb* kindle_fb = findKindleFb();
#ifdef BUILD_FOR_ARM
    if (kindle_fb != 0)
    {
        kindle_fb->forceFullUpdate(fullScreen);
    }
#endif
}

void SystemManager::setFastUpdate(bool fast)
{
    QKindleFb* kindle_fb = findKindleFb();
#ifdef BUILD_FOR_ARM
    if (kindle_fb != 0)
    {
        kindle_fb->setFastUpdate(fast);
    }
#endif
}

void SystemManager::setTransformations(int degree)
{
#ifdef BUILD_FOR_ARM
    qDebug("change screen transformation to %d", degree);
    QWSDisplay::setTransformation(degree / 90);

    // Also write to system conf file.
    const QString PATH = "/DuoKan/setup";
    QFile file(PATH);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("Could not open %s for writing", qPrintable(PATH));
    }
    else
    {
        QString qws_args = file.readAll();
        file.close();

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QString value = qgetenv("QWS_DISPLAY");
            value = value.replace(QRegExp(":Rot\\d+"), QString(":Rot%1").arg(degree));
            QString data;
            qDebug("DuoKan Setup:%s", qPrintable(qws_args));
            if (qws_args.isEmpty())
            {
                data = "export QWS_DISPLAY=" + value;
            }
            else
            {
                //replace Rotxx to Rot&degree
                data = qws_args.replace(QRegExp(":Rot\\d+"), QString(":Rot%1").arg(degree));
            }
            qDebug("Updated Data:%s", qPrintable(data));
            qputenv("QWS_DISPLAY", value.toAscii());
            qDebug("QWS_DISPLAY : %s", qgetenv("QWS_DISPLAY").constData());
            file.write(data.toAscii());
            file.flush();
            file.close();
        }
    }
#endif
}

int SystemManager::screenTransformation()
{
    int degree = 0;
#ifdef BUILD_FOR_ARM
    degree = QScreen::instance()->transformOrientation() * 90;
#endif
    return degree;
}


//void SystemManager::snapshot(const QString &path)
//{
//    screen_manager_.snapshot(path);
//}
//
//bool SystemManager::setGrayScale(int colors)
//{
//    return screen_manager_.setGrayScale(colors);
//}
//
//int SystemManager::grayScale()
//{
//    return screen_manager_.grayScale();
//}

//SystemManagerAdaptor::SystemManagerAdaptor(QApplication &app)
//: QDBusAbstractAdaptor(&app)
//{
//#ifndef _WINDOWS
//    QDBusConnection::systemBus().registerService("com.duokan.service.system_manager");
//    QDBusConnection::systemBus().registerObject("/com/duokan/object/system_manager", &app);
//#else
//    QDBusConnection::sessionBus().registerService("com.duokan.service.system_manager");
//    QDBusConnection::sessionBus().registerObject("/com/duokan/object/system_manager", &app);
//#endif
//
//    // Install keyboard filter.
//#ifdef BUILD_FOR_ARM
//    //QWSServer::addKeyboardFilter(&keyboard_filter_);
//    //QWSServer::setBackground(QBrush(QPixmap(":/images/background_vertical.png")));
//#endif
//
//    // Post startup.
//    QTimer::singleShot(3500, this, SLOT(broadcastInitialSignals(void)));
//
//}
//
//SystemManagerAdaptor::~SystemManagerAdaptor()
//{
//}


