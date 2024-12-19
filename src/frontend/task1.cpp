#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>

#include <clocale>
#include <iostream>

#include "backend_api.h"
#include "ffi.h"
#include "imgui/imgui.h"

typedef unsigned int u32;
typedef float f32;

enum Action {
  kInputTheCount,
  kInputItemsCount,
  kInputUntil,
  kViewWhole,
  kViewSearch,
  kModifyItem,
  kModifyRemove,
  kModifyRemoveAll,
  kNoAction
};

bool InputInterface(bool &open, els::FactoryPart *part, int &filled_in) {
  int material = part->material.tag;
  ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(20, 10), ImGuiCond_FirstUseEver);
  ImGui::Begin("Part input", &open);
  ImGui::Text("Part Name");
  ImGui::InputText("lb0", part->name, sizeof(part->name));

  ImGui::NewLine();
  ImGui::Text("Department No.");
  ImGui::InputInt("lb1", &part->department_no);

  ImGui::NewLine();
  ImGui::Text("Count");
  ImGui::InputInt("lb2", &part->count);

  ImGui::NewLine();
  ImGui::Combo("Material", &material, "Steel\0Brass\0");

  ImGui::NewLine();
  ImGui::Text("Mass");
  ImGui::InputFloat("lb3", &part->weight);

  ImGui::NewLine();
  if (material == 0) {
    part->material.tag = els::Steel;
    ImGui::Text("Mark");
    ImGui::InputInt("lb4", &part->material.steel);
    if (part->material.steel < 3) {
      part->material.steel = 3;
    }
  } else if (material == 1) {
    part->material.tag = els::Brass;
    ImGui::Text("Copper percentage");
    ImGui::InputFloat("lb5", &part->material.brass);
    if (0 >= part->material.brass || part->material.brass >= 100) {
      part->material.brass = 50;
    }
  }

  ImGui::NewLine();
  ImGui::Text("The volume is ....");
  ImGui::NewLine();

  if (ImGui::Button("Submit")) {
    ImGui::End();
    ++filled_in;
    return true;
  }
  if (ImGui::Button("Cancel")) {
    open = false;
  }

  ImGui::End();
  part->material.tag = (els::Material_Tag)material;
  return false;
};

void ViewPart(els::FactoryPart &p, int &index, int count, bool &open) {
  ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(20, 10), ImGuiCond_FirstUseEver);
  ImGui::Begin("Viewing the Part", &open);
  ImGui::Text("Name: %s", p.name);
  ImGui::NewLine();
  ImGui::Text("Department No.: %d", p.department_no);
  ImGui::NewLine();
  ImGui::Text("Count: %d", p.count);
  ImGui::NewLine();
  if (p.material.tag == els::Steel) {
    ImGui::Text("Material: Steel, mark: %d", p.material.steel);
  } else {
    ImGui::Text("Material: Brass, %f%% Copper", p.material.brass);
  }
  ImGui::Text("Volume: ...");
  ImGui::NewLine();
  if (index != 0) {
    if (ImGui::Button("Previous")) {
      --index;
    }
  }
  if (index != count - 1) {
    if (index != 0) {
      ImGui::SameLine();
    }
    if (ImGui::Button("Next")) {
      ++index;
    }
  } else {
    if (ImGui::Button("Finish")) {
      open = false;
    }
  }
  ImGui::End();
}

/// Сортирует массив по убыванию количества
// bool ShakerSort(els::FactoryPart *a, int count) {
//   int sorted_left = 0;
//   int sorted_right = 0;

// }

int main() {
  Elasticsearch* s = els::init_client();
  if (s == nullptr) {
    std::cerr << "Не удалось инициализировать клиент ElasticSearch.\n";
    return 0;
  }
  // Here fetch existing items
  els::FactoryPart *array = nullptr;
  int array_size = 0;
  int filled_in = 0;
  int current_item = 0;

  IMGUI_CHECKVERSION();
  auto ctx = ImGui::CreateContext();
  auto screen = ImTui_ImplNcurses_Init(true, 60);
  ImTui_ImplText_Init();

  bool win_open = true;
  bool action_window_open;
  Action curr_action = kNoAction;

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
          action_window_open = true;
          curr_action = kInputTheCount;
        }
        ImGui::MenuItem("Until met...");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Whole array") && filled_in != 0) {
          current_item = 0;
          curr_action = kViewWhole;
          action_window_open = true;
        }
        ImGui::MenuItem("Search"); // Это шлет запрос
        ImGui::MenuItem("Sort");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Modify")) {
        ImGui::MenuItem("Item");   // Сначала ввести условие
        ImGui::MenuItem("Remove"); // Аналогично
        ImGui::MenuItem("Remove all");
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    switch (curr_action) {
    case kInputTheCount:
      if (action_window_open) {
        int add_size;
        ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);

        ImGui::Begin("Enter the count", &action_window_open);
        ImGui::InputInt("", &add_size);
        if (ImGui::Button("Continue")) {
          els::FactoryPart *new_arr = (els::FactoryPart *)realloc(
              array, sizeof(els::FactoryPart) * (array_size + add_size));
          if (new_arr == nullptr) {
            ImGui::Text("Unable to reallocate memory");
          } else {
            memset(new_arr + array_size, 0,
                   sizeof(els::FactoryPart) * add_size);
            array_size += add_size;
            array = new_arr;
            curr_action = kInputItemsCount;
          }
        }
        ImGui::End();
      }
      break;
    case kInputItemsCount:
      if (action_window_open) {
        InputInterface(action_window_open, array + filled_in, filled_in);
        if (filled_in == array_size) {
          // Здесь же надо послать вновь введенные струкуры на Elastic
          action_window_open = false;
          curr_action = kNoAction;
        }
      } else { // Input is cancelled by the user
        curr_action = kNoAction;
      }
      break;
    case kInputUntil:
      break;
    case kViewWhole:
      if (action_window_open) {
        ViewPart(array[current_item], current_item, filled_in,
                 action_window_open);
      } else {
        curr_action = kNoAction;
      }
      break;
    case kViewSearch:
    case kModifyItem:
    case kModifyRemove:
    case kModifyRemoveAll:
      // послать запрос
      filled_in = 0;
      array_size = 0;
      free(array);
      array = nullptr;
      break;
    case kNoAction:
      action_window_open = false;
      break;
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