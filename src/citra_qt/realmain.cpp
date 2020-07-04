// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <clocale>
#include <fstream>
#include <memory>
#include <thread>
#include <QOpenGLFunctions_3_3_Core>
#ifdef __APPLE__
#include <unistd.h> // for chdir
#endif
#include "citra_qt/applets/mii_selector.h"
#include "citra_qt/applets/swkbd.h"
#include "citra_qt/camera/qt_multimedia_camera.h"
#include "citra_qt/camera/still_image_camera.h"
#include "citra_qt/main.h"
#include "citra_qt/qt_image_interface.h"
#include "common/common_paths.h"
#include "common/detached_tasks.h"
#include "common/file_util.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#ifdef ARCHITECTURE_x86_64
#include "common/x64/cpu_detect.h"
#endif
#include "core/core.h"
#include "core/frontend/applets/default_applets.h"

#ifdef _WIN32
extern "C" {
// tells Nvidia drivers to use the dedicated GPU by default on laptops with switchable graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

#ifdef main
#undef main
#endif

int main(int argc, char* argv[]) {
    Common::DetachedTasks detached_tasks;
    MicroProfileOnThreadCreate("Frontend");
    SCOPE_EXIT({ MicroProfileShutdown(); });

    // Init settings params
    QCoreApplication::setOrganizationName(QStringLiteral("Citra team"));
    QCoreApplication::setApplicationName(QStringLiteral("Citra"));

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapInterval(0);
    // TODO: expose a setting for buffer value (ie default/single/double/triple)
    format.setSwapBehavior(QSurfaceFormat::DefaultSwapBehavior);
    QSurfaceFormat::setDefaultFormat(format);

#ifdef __APPLE__
    std::string bin_path = FileUtil::GetBundleDirectory() + DIR_SEP + "..";
    chdir(bin_path.c_str());
#endif
    QCoreApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);

    // Qt changes the locale and causes issues in float conversion using std::to_string() when
    // generating shaders
    setlocale(LC_ALL, "C");

    GMainWindow main_window;

    // Register CameraFactory
    Camera::RegisterFactory("image", std::make_unique<Camera::StillImageCameraFactory>());
    Camera::RegisterFactory("qt", std::make_unique<Camera::QtMultimediaCameraFactory>());
    Camera::QtMultimediaCameraHandler::Init();

    // Register frontend applets
    Frontend::RegisterDefaultApplets();
    Core::System::GetInstance().RegisterMiiSelector(std::make_shared<QtMiiSelector>(main_window));
    Core::System::GetInstance().RegisterSoftwareKeyboard(std::make_shared<QtKeyboard>(main_window));

    // Register Qt image interface
    Core::System::GetInstance().RegisterImageInterface(std::make_shared<QtImageInterface>());

    main_window.show();

    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &main_window,
                     &GMainWindow::OnAppFocusStateChanged);

    int result = app.exec();
    detached_tasks.WaitForAllTasks();
    return result;
}
