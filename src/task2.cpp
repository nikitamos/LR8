#include <cstring>
#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>

#include <clocale>

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
#include "task2_window.h"
#include "window.h"
#include <QCoreApplication>
#include <QThread>

typedef unsigned int u32;
typedef float f32;

const QString kTask2Index = "task2_library";

int main(int argc, char **argv) {
  QCoreApplication app{argc, argv};
  std::cout << "wainting for debugger";
  getchar();

  Qlastic qls(QUrl("http://localhost:9200/"));
  QlCreateIndex ci{kTask2Index};
  qls.Send(&ci);

  IMGUI_CHECKVERSION();
  auto *ctx = ImGui::CreateContext();
  auto *screen = ImTui_ImplNcurses_Init(true, 60);
  ImTui_ImplText_Init();
  Task2Window mw(&qls);
  mw.SetOpen(true);
  QObject::connect(&mw, &Window::Closed, &app, &QCoreApplication::quit);

  QTimer timer;
  QObject::connect(&timer, &QTimer::timeout, [&mw, screen]() {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);
    mw.Render();

    ImGui::Render();

    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
    ImTui_ImplNcurses_DrawScreen();
  });

  // ImTui_ImplText_Shutdown();
  // ImTui_ImplNcurses_Shutdown();
  timer.setInterval(1000 / 30);
  timer.start();
  return QCoreApplication::exec();
}