#include "task1_mainwindow.h"
#include "backend_api.h"
#include "metamagic.h"
#include <imgui/imgui.h>
#include <qobject.h>

class CustomViewAction;
void ViewPart(FactoryPart &p, int &index, int count, bool &open,
              CustomViewAction *act = nullptr);

/*
class SearchInput {
public:
  SearchInput() { memset(&part_, 0, sizeof(part_)); }
  void Render(bool &open) {
    if (IsSubmitted()) {
      if (res_.count == 0) {
        ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(20, 10), ImGuiCond_FirstUseEver);
        ImGui::Begin("SearchResults", &open);
        ImGui::Text("Nothing found :(");
        ImGui::End();
      } else {
        ViewPart(res_.res[index_], index_, res_.count, open);
        if (!open) {
          submit_ = false;
        }
      }
      return;
    }
    ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(20, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("Win", &open);
    ImGui::Combo("##FieldCombo", &field_,
                 "Name\0Department\0Weight\0Material\0Volume\0Count\0");
    ImGui::NewLine();
    switch (field_) {
    case 0:
      preview_ = "Name";
      ImGui::InputText("##LIAUSTF", part_.name, sizeof(part_.name));
      break;
    case 1:
      preview_ = "Department";
      ImGui::InputInt("##er;orty", &part_.department_no);
      break;
    case 2:
      preview_ = "Weight";
      ImGui::InputFloat("##:LKUAry934y", &part_.weight);
      break;
    case 3:
      preview_ = "Material";
      ImGui::Combo("##MaterialCombo", &material_tag_, "Steel\0Brass\0");
      if (material_tag_ == 0) {
        ImGui::Text("Mark");
        ImGui::SameLine();
        ImGui::InputInt("##as;eor", &part_.material.steel);
      } else {
        ImGui::Text("Copper percentage");
        ImGui::SameLine();
        ImGui::InputFloat("##w[0sfdugh]", &part_.material.brass);
      }
      break;
    case 4:
      preview_ = "Volume";
      ImGui::InputFloat("##sldfih", &part_.volume);
      break;
    case 5:
      preview_ = "Count";
      ImGui::InputInt("##eqeitj", &part_.count);
      break;
    default:
      break;
    }
    ImGui::NewLine();
    if (ImGui::Button("Submit")) {
      submit_ = true;
      open = false;
    }
    ImGui::End();
  }

  void Send(Elasticsearch *client) {
    // res_ = els::search_for_part(client, &part_, field_);
  }
  bool IsMatch(FactoryPart &part) const {
    switch (field_) {
    case 0:
      return std::strcmp(part.name, part_.name) != 0;
    case 1:
      return part.department_no == part_.department_no;
    case 2:
      return part.weight == part_.weight;
    case 3:
      if (part_.material.tag == part.mt) {
        if (part.material.tag == els::Material_Tag::Steel) {
          return part_.material.steel == part.material.steel;
        }
        return part_.material.brass == part.material.brass;
      }
      return false;
    case 4:
      return part.volume == part_.volume;
    case 5:
      return part.count == part_.count;
    default:
      std::cerr << "WARNING: ...\n";
      return false;
    }
  }
  bool Unsubmit() { submit_ = false; }
  bool IsSubmitted() const { return submit_; }
  void Deallocate() { els::free_all_parts(&res_.res, &res_.count); }

private:
//   els::FactoryPart part_;
  int material_tag_ = 0;
  int field_ = 0;
  int index_ = 0;
  bool submit_ = false;
  const char *preview_ = "Select the field for search";
//   els::PartSearchResult res_{nullptr, 0};
};*/

class CustomViewAction {
public:
  explicit CustomViewAction(const char *button_text) : text_(button_text) {}
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
  DeleteAction(Elasticsearch *client, els::FactoryPart *&arr, int &size,
               int &filled_in)
      : CustomViewAction("Delete"), array_(arr), size_(size),
        filled_in_(filled_in), client_(client) {}
  virtual void Execute(int &index, els::FactoryPart &part) override {
    els::delete_factory_document(client_, &part);
    for (int i = index; i < size_ - 1; ++i) {
      std::memcpy(array_ + i, array_ + i + 1, sizeof(els::FactoryPart));
    }
    array_ = static_cast<els::FactoryPart *>(
        realloc(array_, (--filled_in_, --size_)));
  }

private:
  els::FactoryPart *&array_;
  int &size_;
  int &filled_in_;
  Elasticsearch *client_;
};

void ViewPart(els::FactoryPart &p, int &index, int count, bool &open,
              CustomViewAction *act) {
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
  if (act != nullptr) {
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
void ShakerSort(FactoryPart *a, int count) {
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
    for (int i = right; i > left; --i) {
      if (a[i].count > a[i - 1].count) {
        std::swap(a[i], a[i - 1]);
        last_swap = i - 1;
      }
    }
    left = last_swap;
  }
}

void Task1MainWindow::DrawMenuWindow() {
  auto io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 6, io.DisplaySize.y - 3),
                           ImGuiCond_Always);

  ImGui::Begin("Database of a factory", &win_open_, ImGuiWindowFlags_MenuBar);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Input")) {
      if (ImGui::MenuItem("The count")) {
        action_win_open_ = true;
        curr_action_ = kInputTheCount;
      }
      if (ImGui::MenuItem("Until condition")) {
        action_win_open_ = true;
        curr_action_ = kInputUntil;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Whole array") && filled_in_ != 0) {
        curr_item_ = 0;
        curr_action_ = kViewWhole;
        action_win_open_ = true;
      }
      if (ImGui::MenuItem("Search")) {
        curr_action_ = kViewSearch;
        action_win_open_ = true;
      }
      if (ImGui::MenuItem("Sort")) {
        // ShakerSort(array, filled_in);
        curr_action_ = kViewWhole;
        action_win_open_ = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Modify")) {
      ImGui::MenuItem("Item"); // Сначала ввести условие
      if (ImGui::MenuItem("Remove")) {
        curr_action_ = kModifyRemove;
        action_win_open_ = true;
      }
      if (ImGui::MenuItem("Remove all")) {
        curr_action_ = kModifyRemoveAll;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  ImGui::End();
}

void Task1MainWindow::PartInputSubmitted(QObject *part) {
  ++filled_in_;
  create_.AddDocument(&part_wrapper_);
  if (filled_in_ == array_size_) {
    // Input is finished. Send the data to Elastic
    qls_->Send(&create_);
    curr_action_ = kNoAction;
    curr_item_ = 0;
  } else {
    part_wrapper_.SetTarget(&array_[++curr_item_]);
  }
}

void Task1MainWindow::Render() {
  auto io = ImGui::GetIO();

  switch (curr_action_) {
  case kInputTheCount:
    if (action_win_open_) {
      int add_size = 2;
      ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);

      ImGui::Begin("Enter the count", &action_win_open_);
      ImGui::InputInt("", &add_size);
      if (ImGui::Button("Continue")) {
        if (add_size == 0) {
          curr_action_ = kNoAction;
        } else {
          FactoryPart *new_arr = static_cast<FactoryPart *>(
              realloc(array_, sizeof(FactoryPart) * (array_size_ + add_size)));
          if (new_arr == nullptr) {
            ImGui::Text("Unable to reallocate memory ");
          } else {
            memset(new_arr + array_size_, 0, sizeof(FactoryPart) * add_size);
            old_size_ = array_size_;
            array_size_ += add_size;
            array_ = new_arr;
            curr_action_ = kInputItemsCount;
            curr_item_ = old_size_;
          }
        }
      }
      ImGui::End();
    } else {
      curr_action_ = kNoAction;
    }
    break;
  case kInputItemsCount:
    if (action_win_open_) {
      meta_input_.Render();
    } else { // Input is cancelled by the user. Send nothing and free unused
             // memory
      array_size_ = old_size_;
      filled_in_ = old_size_;
      array_ = static_cast<FactoryPart *>(
          realloc(array_, array_size_ * sizeof(FactoryPart)));
      curr_action_ = kNoAction;
    }
    break;
  case kInputUntil:
    break;
    // if (action_win_open) {
    //   search_input.Render(action_win_open);
    // } else {
    //   if (search_input.IsSubmitted()) {
    //     int x;
    //     InputInterface(action_win_open, &buffer, x);
    //   } else {
    //     curr_action = kNoAction;
    //   }
    // }
  case kViewWhole:
    if (action_win_open_) {
      //   ViewPart(array[curr_item], curr_item, filled_in, action_win_open);
    } else {
      curr_action_ = kNoAction;
    }
    break;
  case kViewSearch:
    break;
    // if (action_win_open) {
    //   int index = 0;
    //   search_input.Render(action_win_open);
    // } else {
    //   if (search_input.IsSubmitted()) {
    //     search_input.Send(client);
    //     action_win_open = true;
    //   } else {
    //     curr_action = kNoAction;
    //   }
    // }
  case kModifyItem:
    break;
  case kModifyRemove:

    // if (action_win_open) {
    //   ViewPart(array[curr_item], curr_item, filled_in, action_win_open,
    //            &action_remove);
    // } else {
    //   curr_action = kNoAction;
    // }
    break;
  case kModifyRemoveAll:
    filled_in_ = 0;
    curr_item_ = 0;
    if (false /*delete_all_from_index(client, &array, &array_size) != 1*/) {
      exit(1);
    } else {
      curr_action_ = kNoAction;
    }
    break;
  case kNoAction:
    action_win_open_ = false;
    DrawMenuWindow();
    break;
  }
}
