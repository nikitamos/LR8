#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>

#include <clocale>
#include <iostream>
#include <utility>

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

class CustomViewAction {
public:
  constexpr CustomViewAction(const char *button_text) : text_(button_text) {}
  virtual void Execute(int &index, els::FactoryPart &part) = 0;
  void Render(int &index, els::FactoryPart &part) {
    if (ImGui::Button(text_)) {
      Execute(index, part);
    }
  }

private:
  const char *text_;
};

class DeleteAction : public CustomViewAction {
public:
  constexpr DeleteAction() : CustomViewAction("Delete") {}
  virtual void Execute(int &index, els::FactoryPart &part) override {}
};

static constinit DeleteAction action_remove;

void ViewPart(els::FactoryPart &p, int &index, int count, bool &open,
              CustomViewAction *act = nullptr) {
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
  if (act) {
    ImGui::SameLine();
    act->Render(index, p);
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
/// (за что этот алгоритм?)
void ShakerSort(els::FactoryPart *a, int count) {
  int left = 0;
  int right = count - 1;
  while (left != right) {
    int last_swap = left;
    for (int i = left; i < right; ++i) {
      if (a[i].count < a[i + 1].count) {
        std::swap(a[i], a[i + 1]);
        last_swap = i + 1;
      }
    }
    right = last_swap;
    for (int i = right; i > left; ++i) {
      if (a[i].count > a[i - 1].count) {
        std::swap(a[i], a[i - 1]);
        last_swap = i - 1;
      }
    }
    left = last_swap;
  }
}

void DrawMenuWindow(Action &curr_action, bool &win_open, bool &action_win_open,
                    int &curr_item, int &filled_in, els::FactoryPart *array) {
  auto io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 6, io.DisplaySize.y - 3),
                           ImGuiCond_Always);

  ImGui::Begin("Database of a factory", &win_open, ImGuiWindowFlags_MenuBar);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Input")) {
      if (ImGui::MenuItem("Specific count")) {
        action_win_open = true;
        curr_action = kInputTheCount;
      }
      ImGui::MenuItem("Until met...");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Whole array") && filled_in != 0) {
        curr_item = 0;
        curr_action = kViewWhole;
        action_win_open = true;
      }
      ImGui::MenuItem("Search"); // Это шлет запрос
      if (ImGui::MenuItem("Sort")) {
        ShakerSort(array, filled_in);
        curr_action = kViewWhole;
        action_win_open = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Modify")) {
      ImGui::MenuItem("Item");   // Сначала ввести условие
      if (ImGui::MenuItem("Remove")) {
        curr_action = kModifyRemove;
        action_win_open = true;
      }
      if (ImGui::MenuItem("Remove all")) {
        curr_action = kModifyRemoveAll;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  ImGui::End();
}

int main() {
#ifndef NDEBUG
  std::cout << "Waiting for debugger";
  getchar();
#endif
  Elasticsearch *client = els::init_client();
  if (client == nullptr) {
    std::cout << "Error creating Elasticsearch client\n";
    return 1;
  }
  // Here fetch existing items
  els::FactoryPart *array = nullptr;
  CustomViewAction *view_action = nullptr;
  int old_size = 0;
  int array_size = 0;
  int filled_in = 0;
  int curr_item = 0;

  els::retrieve_all(client, &array, &array_size);
  if (array_size < 0) {
    std::cout << "Error connecting to Elasticsearch\n";
    els::close_client(client);
    return 1;
  }
  filled_in = array_size;

  IMGUI_CHECKVERSION();
  auto ctx = ImGui::CreateContext();
  auto screen = ImTui_ImplNcurses_Init(true, 60);
  ImTui_ImplText_Init();

  bool win_open = true;
  bool action_win_open;
  Action curr_action = kNoAction;

  while (win_open) {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    setlocale(LC_ALL, "ru_RU.UTF-8");

    auto io = ImGui::GetIO();

    switch (curr_action) {
    case kInputTheCount:
      if (action_win_open) {
        int add_size;
        ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);

        ImGui::Begin("Enter the count", &action_win_open);
        ImGui::InputInt("", &add_size);
        if (ImGui::Button("Continue")) {
          if (add_size == 0) {
            curr_action = kNoAction;
          } else {
            els::FactoryPart *new_arr = (els::FactoryPart *)realloc(
                array, sizeof(els::FactoryPart) * (array_size + add_size));
            if (new_arr == nullptr) {
              ImGui::Text("Unable to reallocate memory");
            } else {
              memset(new_arr + array_size, 0,
                     sizeof(els::FactoryPart) * add_size);
              old_size = array_size;
              array_size += add_size;
              array = new_arr;
              curr_action = kInputItemsCount;
            }
          }
        }
        ImGui::End();
      } else {
        curr_action = kNoAction;
      }
      break;
    case kInputItemsCount:
      if (action_win_open) {
        InputInterface(action_win_open, array + filled_in, filled_in);
        if (filled_in ==
            array_size) { // Input is finished. Send data to Elastic
          els::add_parts(client, array + old_size, filled_in - old_size);
          action_win_open = false;
          curr_action = kNoAction;
        }
      } else { // Input is cancelled by the user. Send nothing and free unused
               // memory
        array_size = old_size;
        filled_in = old_size;
        array = (els::FactoryPart *)realloc(
            array, array_size * sizeof(els::FactoryPart));
        curr_action = kNoAction;
      }
      break;
    case kInputUntil:
      break;
    case kViewWhole:
      if (action_win_open) {
        ViewPart(array[curr_item], curr_item, filled_in, action_win_open);
      } else {
        curr_action = kNoAction;
      }
      break;
    case kViewSearch:
    case kModifyItem:
    case kModifyRemove:
      if (action_win_open) {
        ViewPart(array[curr_item], curr_item, filled_in, action_win_open,
                 &action_remove);
      } else {
        curr_action = kNoAction;
      }
      break;
    case kModifyRemoveAll:
      filled_in = 0;
      curr_item = 0;
      if (els::delete_all_from_index(client, &array, &array_size) != 1) {
        exit(1);
      } else {
        curr_action = kNoAction;
      }
      break;
    case kNoAction:
      action_win_open = false;
      DrawMenuWindow(curr_action, win_open, action_win_open, curr_item,
                     filled_in, array);
      break;
    }
    ImGui::Render();

    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
    ImTui_ImplNcurses_DrawScreen();
  }

  ImTui_ImplText_Shutdown();
  ImTui_ImplNcurses_Shutdown();

  els::free_all_parts(&array, &array_size);
  els::close_client(client);

  return 0;
}