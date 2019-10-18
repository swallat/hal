#include "plugin_gui.h"

#include "core/log.h"
#include "core/plugin_manager.h"
#include "core/utils.h"

#include "netlist/gate_library/gate_library_manager.h"

#include "gui/file_manager/file_manager.h"
#include "gui/file_status_manager/file_status_manager.h"
#include "gui/graph_widget/graph_context_manager.h"
#include "gui/hal_content_manager/hal_content_manager.h"
#include "gui/main_window/main_window.h"
#include "gui/netlist_relay/netlist_relay.h"
#include "gui/notifications/notification_manager.h"
#include "gui/plugin_management/plugin_relay.h"
#include "gui/python/python_context.h"
#include "gui/selection_relay/selection_relay.h"
#include "gui/settings/settings_manager.h"
#include "gui/settings/settings_relay.h"
#include "gui/style/style.h"
#include "gui/thread_pool/thread_pool.h"
#include "gui/window_manager/window_manager.h"

#include <signal.h>

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QMetaType>
#include <QResource>
#include <QSettings>
#include <QString>
#include <gui/focus_logger/focus_logger.h>

QSettings g_settings(QString::fromStdString((core_utils::get_user_config_directory() / "/guisettings.ini").string()), QSettings::IniFormat);
QSettings g_gui_state(QString::fromStdString((core_utils::get_user_config_directory() / "/guistate.ini").string()), QSettings::IniFormat);

settings_manager g_settings_manager;

window_manager* g_window_manager;
notification_manager* g_notification_manager;

hal_content_manager* g_content_manager = nullptr;

std::shared_ptr<netlist> g_netlist = nullptr;

netlist_relay g_netlist_relay;
plugin_relay g_plugin_relay;
selection_relay g_selection_relay;
settings_relay g_settings_relay;

file_status_manager g_file_status_manager;

thread_pool* g_thread_pool;

graph_context_manager g_graph_context_manager;

std::unique_ptr<python_context> g_python_context = nullptr;

// NOTE
// ORDER = LOGGER -> SETTINGS -> (STYLE / RELAYS / OTHER STUFF) -> MAINWINDOW (= EVERYTHING ELSE & DATA)
// USE POINTERS FOR EVERYTHING ?

static void handle_program_arguments(const program_arguments& args)
{
    if (args.is_option_set("--input-file"))
    {
        auto file_name = hal::path(args.get_parameter("--input-file"));
        log_info("gui", "GUI started with file {}.", file_name.string());
        file_manager::get_instance()->open_file(QString::fromStdString(file_name.string()));
    }
}

static void cleanup()
{
    delete g_notification_manager;
    //    delete g_window_manager;
}

static void m_cleanup(int sig)
{
    if (sig == SIGINT)
    {
        log_info("gui", "Detected Ctrl+C in terminal");
        QApplication::exit(0);
    }
}

bool plugin_gui::exec(program_arguments& args)
{
    int argc;
    const char** argv;
    args.get_original_arguments(&argc, &argv);
    QApplication a(argc, const_cast<char**>(argv));
    focus_logger focusLogger(&a);

    QObject::connect(&a, &QApplication::aboutToQuit, cleanup);

    QApplication::setApplicationName("HAL Qt Interface");
    QApplication::setOrganizationName("Chair for Embedded Security - Ruhr University Bochum");
    QApplication::setOrganizationDomain("emsec.rub.de");

// Non native dialogs does not work on macOS. Therefore do net set AA_DontUseNativeDialogs!
#ifdef __linux__
    a.setAttribute(Qt::AA_DontUseNativeDialogs, true);
#endif
    a.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QResource::registerResource("gui_resources.rcc");

    QFontDatabase::addApplicationFont(":/fonts/Cabin-Bold");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-BoldItalic");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-Italic");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-Medium");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-MediumItalic");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-Regular");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-SemiBold");
    QFontDatabase::addApplicationFont(":/fonts/Cabin-SemiBoldItalic");
    QFontDatabase::addApplicationFont(":/fonts/Hack-Bold");
    QFontDatabase::addApplicationFont(":/fonts/Hack-BoldItalic");
    QFontDatabase::addApplicationFont(":/fonts/Hack-Regular");
    QFontDatabase::addApplicationFont(":/fonts/Hack-RegularItalic");
    QFontDatabase::addApplicationFont(":/fonts/Hack-RegularOblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-bold");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-bolditalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-boldoblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-extralight");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-extralightitalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-extralightoblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-heavy");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-heavyitalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-heavyoblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-italic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-light");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-lightitalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-lightoblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-medium");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-mediumitalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-mediumoblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-oblique");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-regular");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-thin");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-thinitalic");
    QFontDatabase::addApplicationFont(":/fonts/iosevka-thinoblique");
    QFontDatabase::addApplicationFont(":/fonts/Droid Sans Mono/DroidSansMono");
    QFontDatabase::addApplicationFont(":/fonts/Montserrat/Montserrat-Black");
    QFontDatabase::addApplicationFont(":/fonts/Source Code Pro/SourceCodePro-Black");

    // LOGGER HERE

    gate_library_manager::load_all();

    // TEST
    //    g_settings.setValue("stylesheet/base", ":/style/test base");
    //    g_settings.setValue("stylesheet/definitions", ":/style/test definitions2");
    //    a.setStyleSheet(style::get_stylesheet());

    //TEMPORARY CODE TO CHANGE BETWEEN THE 2 STYLESHEETS WITH SETTINGS (NOT FINAL)
    //this settingsobject is currently neccessary to read from the settings from here, because the g_settings are not yet initialized(?)
    QSettings tempsettings_to_read_from(QString::fromStdString((core_utils::get_user_config_directory() / "/guisettings.ini").string()), QSettings::IniFormat);
    QString stylesheet_to_open = ":/style/darcula";    //default style

    if (tempsettings_to_read_from.value("main_style/theme", "") == "" || tempsettings_to_read_from.value("main_style/theme", "") == "darcula")
        stylesheet_to_open = ":/style/darcula";
    else if (tempsettings_to_read_from.value("main_style/theme", "") == "sunny")
        stylesheet_to_open = ":/style/sunny";

    QFile stylesheet(stylesheet_to_open);
    stylesheet.open(QFile::ReadOnly);
    a.setStyleSheet(QString(stylesheet.readAll()));
    stylesheet.close();
    //##############END OF TEMPORARY TESTING TO SWITCH BETWEEN STYLESHEETS

    style::debug_update();

    qRegisterMetaType<spdlog::level::level_enum>("spdlog::level::level_enum");

    //    g_window_manager       = new window_manager();
    g_notification_manager = new notification_manager();

    g_thread_pool = new thread_pool();

    signal(SIGINT, m_cleanup);

    main_window w;
    handle_program_arguments(args);
    w.show();
    auto ret = a.exec();
    return ret;
}

std::string plugin_gui::get_name()
{
    return std::string("hal_gui");
}

std::string plugin_gui::get_version()
{
    return std::string("0.1");
}

std::set<interface_type> plugin_gui::get_type()
{
    return {interface_type::base, interface_type::interactive_ui};
}

void plugin_gui::initialize_logging()
{
    log_manager& l = log_manager::get_instance();
    l.add_channel("user", {log_manager::create_stdout_sink(), log_manager::create_file_sink(), log_manager::create_gui_sink()}, "info");
    l.add_channel("gui", {log_manager::create_stdout_sink(), log_manager::create_file_sink(), log_manager::create_gui_sink()}, "info");
    l.add_channel("python", {log_manager::create_stdout_sink(), log_manager::create_file_sink(), log_manager::create_gui_sink()}, "info");
}
