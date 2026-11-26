#include "capturer.h"
#include "config.h"
#include "logging.h"
#include "probe/cpu.h"
#include "probe/system.h"
#include "probe/util.h"
#include "version.h"
#include "helper.h"

#include <QTranslator>

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

#ifdef _WIN32
    ::SetConsoleOutputCP(CP_UTF8);
    ::setvbuf(stdout, nullptr, _IONBF, 0);
#endif

    // glog
    Logger::init(argv[0]);

    probe::thread::set_name("sunnycapturer-main");

    Capturer app(argc, argv);
    Capturer::setQuitOnLastWindowClosed(false);

#ifdef _MSC_VER
    // SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)Helper::ApplicationCrashHandler); //注冊异常捕获函数
#endif

    // void* p = nullptr;
    // int* q = static_cast<int*>(p); // 将 void* 转换为 int*
    // *q = 42;  // 试图写入 nullptr 地址，会导致崩溃（访问冲突）

    config::load();

    logi("SunnyCapturer          {}", CAPTURER_VERSION);
    logi(" -- Qt               : {}", qVersion());
    logi(" -- Operating System : {} ({})", probe::system::name(),
         probe::to_string(probe::system::version()));
    logi(" -- Kernel           : {} {}", probe::system::kernel::name(),
         probe::to_string(probe::system::kernel::version()));
    logi(" -- CPU              : {}", probe::cpu::info().name);
    logi(" -- Architecture     : {}", probe::to_string(probe::cpu::architecture()));
    logi(" -- Virtual Screen   : {}", probe::to_string(probe::graphics::virtual_screen_geometry()));
    for (const auto& display : probe::graphics::displays()) {
        logi(" --   {:>14} : {}", display.id, probe::to_string(display.geometry));
    }
    logi(" -- Windowing System : {}", probe::to_string(probe::system::windowing_system()));
    logi(" -- Desktop ENV      : {} ({})", probe::to_string(probe::system::desktop_environment()),
         probe::to_string(probe::system::desktop_environment_version()));
    logi(" -- Language         : {}", config::general::language.toStdString());
    logi(" -- Config File      : {}", config::general::filepath.toStdString());
    logi("");

    QTranslator translator;
    const auto translation = Helper::isLanguageAvailable(config::general::language);
    const auto okLoad = translator.load(translation);
    const auto okInstallTranslator = qGuiApp->installTranslator(&translator);
    loge_if(!okLoad, "failed to load {}", translation.toStdString());
    loge_if(!okInstallTranslator, "failed to installTranslator {}", translation.toStdString());
    logi(" -- qm load          : {}", okLoad ? "true" : "false");
    logi(" -- installTranslator: {}", okInstallTranslator ? "true" : "false");

    app.Init();

    return Capturer::exec();
}
