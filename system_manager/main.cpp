// -*- mode: c++; c-basic-offset: 4; -*-

#include <QtGui/QApplication>
#include <QString>
#include <QtGui/qscreen_qws.h>
#include <QtGui/qwsdisplay_qws.h>

#include "System/inc/system_manager.h"


int main(int argc, char* argv[])
{
    // We have to call this before Qt use the tslib plugin.
    // Otherwise, tslib can not initialize the touch screen.
    QApplication app(argc, argv);
    //SystemManagerAdaptor adaptor(app);
    SystemManager::instance();

    return app.exec();
}
