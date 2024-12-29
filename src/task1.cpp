#include <cstring>
#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>

#include <QTimer>
#include <iostream>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qjsondocument.h>
#include <qlogging.h>
#include <qmetaobject.h>
#include <qnetworkreply.h>
#include <qobject.h>
#include <qtimer.h>
#include <qvariant.h>

#include "imgui/imgui.h"
#include "qlastic.h"
#include "task1_mainwindow.h"
#include "window.h"
#include <QCoreApplication>
#include <QThread>

const QString kTask1Index = "task1_factory";

int main(int argc, char **argv) {
  QCoreApplication app{argc, argv};
  std::cout << "wainting for debugger";
  getchar();

  Qlastic qls(QUrl("http://localhost:9200/"));

  IMGUI_CHECKVERSION();
  auto *ctx = ImGui::CreateContext();
  auto *screen = ImTui_ImplNcurses_Init(true, 60);
  ImTui_ImplText_Init();
  Task1Window mw(&qls);
  mw.SetOpen(true);
  QObject::connect(&mw, &Window::Closed, &app, &QCoreApplication::quit);

  QTimer timer;
  bool finish = false;
  bool *finptr = &finish;
  auto conn =
      QObject::connect(&timer, &QTimer::timeout, [&mw, screen, finptr]() {
        if (*finptr) {
          return;
        }
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);
        mw.Render();

        ImGui::Render();

        auto *data = ImGui::GetDrawData();
        if (data != nullptr && !*finptr) {
          ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
          ImTui_ImplNcurses_DrawScreen();
        }
      });

  timer.setInterval(1000 / 30);
  timer.start();

  QObject::connect(&mw, &Window::Closed, &app, QCoreApplication::quit);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&conn, finptr]() {
    *finptr = true;
    // QObject::disconnect(conn);
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
  });

  return QCoreApplication::exec();
}
