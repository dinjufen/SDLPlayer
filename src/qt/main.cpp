#include "qt/PlayerController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("SDLPlayer"));
    application.setApplicationDisplayName(QStringLiteral("SDLPlayer"));
    application.setOrganizationName(QStringLiteral("SDLPlayer"));

    // Avoid Material's default purple/green; the QML theme draws its own chrome.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    PlayerController player;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("player"), &player);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &application,
        [](QObject* object, const QUrl&) {
            if (object == nullptr) {
                QCoreApplication::exit(1);
            }
        },
        Qt::QueuedConnection);

    engine.loadFromModule("SDLPlayer", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    const QStringList arguments = application.arguments();
    if (arguments.size() > 1 && !arguments.at(1).startsWith(QLatin1Char('-'))) {
        player.openPath(arguments.at(1));
    }

    return application.exec();
}
