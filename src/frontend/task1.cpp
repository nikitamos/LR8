#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>

#include <clocale>
#include <iostream>

#include "backend_api.h"

typedef unsigned int u32;
typedef float f32;


void InputInterface(bool& open, els::FactoryPart& part) {
  int material = part.material.tag;
  ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(20, 10), ImGuiCond_FirstUseEver);
  ImGui::Begin("Part input", &open);
  char buf[80];
  ImGui::Text("Part Name");
  ImGui::InputText("", part.name, sizeof(part.name));
  
  ImGui::NewLine();
  ImGui::Text("Department No.");
  ImGui::InputInt("", &part.department_no);

  ImGui::NewLine();
  ImGui::Text("Count");
  ImGui::InputInt3("", &part.count);

  ImGui::NewLine();
  ImGui::Text("Material");

  ImGui::Combo("Material", &material, "Steel\0Brass\0");
  ImGui::End();
  part.material.tag = (els::Material_Tag) material;
  
};


int main() {
  els::FactoryPart p;
  Elasticsearch* s = els::init_client();
  if (s == nullptr) {
    std::cerr << "Не удалось инициализировать клиент ElasticSearch.\n";
    return 0;
  }
  IMGUI_CHECKVERSION();
  auto ctx = ImGui::CreateContext();
  auto screen = ImTui_ImplNcurses_Init(true, 60);
  ImTui_ImplText_Init();

  bool win_open = true;
  bool input_open = false;

  while (win_open) {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    setlocale(LC_ALL, "ru_RU.UTF-8");

    auto io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x-6, io.DisplaySize.y-3), ImGuiCond_Always);

    ImGui::Begin("Database of a factory", &win_open, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Input")) {
        if (ImGui::MenuItem("Specific count")) {
          input_open = true;
        }
        ImGui::MenuItem("Until met...");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Whole array");
        ImGui::MenuItem("Search");
        ImGui::MenuItem("Sort");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Modify")) {
        ImGui::MenuItem("Item");
        ImGui::MenuItem("Remove");
        ImGui::MenuItem("Remove all");
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    } else {
      ImGui::BulletText("Fuckup!");
    }

    if (input_open) {
      InputInterface(input_open, p);
    }

    ImGui::End();

    ImGui::Render();

    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
    ImTui_ImplNcurses_DrawScreen();
  }

  ImTui_ImplText_Shutdown();
  ImTui_ImplNcurses_Shutdown();

  els::close_client(s);

  return 0;
}