#include "task1_mainwindow.h"
#include "backend_api.h"
#include "metamagic.h"
#include "qlastic.h"
#include "task1part.h"
#include <cstdlib>
#include <format>
#include <imgui/imgui.h>
#include <qcoreapplication.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qobject.h>

class CustomViewAction;
void ViewPart(FactoryPart &p, int &index, int count, bool &open,
              CustomViewAction *act = nullptr);

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

/// Сортирует массив по убыванию количества
/// (за что этот алгоритм?)
void ShakerSort(FactoryPart *a, int count) {
  if (count <= 1) {
    return;
  }
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

void Task1Window::DrawMenuWindow() {
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
        meta_viewer_.SetCollectionSize(filled_in_);
        meta_viewer_.SetProvider(&part_wrapper_);
        part_wrapper_.SetTarget(array_);
        meta_viewer_.SetCurrent(0);
        curr_item_ = 0;
        curr_action_ = kViewWhole;
        action_win_open_ = true;
      }
      if (ImGui::MenuItem("Sort")) {
        ShakerSort(array_, filled_in_);
        curr_action_ = kViewWhole;
        action_win_open_ = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Select Items")) {
      curr_action_ = kInputProperty;
      property_selector_.SetOpen(true);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  ImGui::Text("%s", text_.c_str());
  ImGui::End();
}

void Task1Window::PartInputSubmitted(QObject * /*unused*/) {
  ++filled_in_;
  create_.AddDocument(&part_wrapper_);
  if (filled_in_ == array_size_) {
    // Input is finished. Send the data to Elastic
    qls_->Send(&create_);
    curr_action_ = kWait;
    curr_item_ = 0;
  } else {
    part_wrapper_.SetTarget(&array_[++curr_item_]);
  }
}

void Task1Window::PartInputCancelled() {
  array_size_ = old_size_;
  filled_in_ = old_size_;
  array_ = static_cast<FactoryPart *>(
      realloc(array_, array_size_ * sizeof(FactoryPart)));
  curr_action_ = kNoAction;
}

void Task1Window::Render() {
  auto io = ImGui::GetIO();

  switch (curr_action_) {
  case kInputTheCount:
    if (action_win_open_) {
      ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);

      ImGui::Begin("Enter the count", &action_win_open_);
      ImGui::InputInt("", &add_size_);
      if (ImGui::Button("Continue")) {
        if (add_size_ == 0) {
          curr_action_ = kNoAction;
        } else {
          FactoryPart *new_arr = static_cast<FactoryPart *>(
              realloc(array_, sizeof(FactoryPart) * (array_size_ + add_size_)));
          if (new_arr == nullptr) {
            ImGui::Text("Unable to reallocate memory ");
          } else {
            memset(new_arr + array_size_, 0, sizeof(FactoryPart) * add_size_);
            old_size_ = array_size_;
            array_size_ += add_size_;
            array_ = new_arr;
            curr_action_ = kInputItemsCount;
            curr_item_ = old_size_;
            meta_input_.Reset();
          }
        }
      }
      ImGui::End();
    } else {
      curr_action_ = kNoAction;
    }
    break;
  case kInputItemsCount:
    meta_input_.Render();
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
    meta_viewer_.Render();
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
    // field_selector_.Render();
    break;
  case kModifyRemove:
    property_selector_.Render();
    // if (action_win_open) {
    //   ViewPart(array[curr_item], curr_item, filled_in, action_win_open,
    //            &action_remove);
    // } else {
    //   curr_action = kNoAction;
    // }
    break;
  case kModifyRemoveAll:
    qls_->Send(&index_delete_);
    break;
  case kNoAction:
    action_win_open_ = false;
    DrawMenuWindow();
    break;
  case kWait:
    ImGui::SetNextWindowPos(ImVec2(30, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(36, 4), ImGuiCond_FirstUseEver);
    ImGui::Begin("Wait for a while...");
    ImGui::Text("Your request is being processed...");
    ImGui::End();
    break;
  case kInputProperty:
    property_selector_.Render();
    break;
  }
}

Task1Window::Task1Window(Qlastic *qls, QObject *parent)
    : Window(parent), part_wrapper_(&buf_), qls_(qls),
      property_selector_(MetaFactoryPart::staticMetaObject.metaType()) {
  meta_input_.SetTarget(&part_wrapper_);
  QObject::connect(&meta_input_, &MetaInput::Submit, this,
                   &Task1Window::PartInputSubmitted);
  QObject::connect(&meta_input_, &MetaInput::Cancel, this,
                   &Task1Window::PartInputCancelled);
  QObject::connect(&create_, &QlBulkCreateDocuments::Success, this,
                   &Task1Window::PartsCreated);
  QObject::connect(&create_, &QlBulkCreateDocuments::Failure, this,
                   &Task1Window::PartsCreationFailed);
  QObject::connect(&meta_viewer_, &MetaViewer::Next, this,
                   &Task1Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Previous, this,
                   &Task1Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Closed, this,
                   &Task1Window::CancelAction);
  QObject::connect(&search_, &QlSearch::Success, this,
                   &Task1Window::SearchSucceed);
  QObject::connect(&search_, &QlSearch::Failure, this,
                   &Task1Window::SearchFailed);

  QObject::connect(&index_delete_, &QlDeleteIndex::Success, this,
                   &Task1Window::IndexDeleted);
  QObject::connect(&index_delete_, &QlDeleteIndex::Failure, this,
                   &Task1Window::IndexDeleteFailed);
  QObject::connect(&index_create_, &QlCreateIndex::Success, this,
                   &Task1Window::IndexCreated);
  QObject::connect(&index_create_, &QlCreateIndex::Failure, this,
                   &Task1Window::IndexCreateFailed);

  QObject::connect(&property_selector_, &FieldValueSelector::Cancel, this,
                   &Task1Window::CancelAction);
  QObject::connect(&property_selector_, &FieldValueSelector::Search, this,
                   &Task1Window::SendSearch);
  qls_->Send(&search_);
  curr_action_ = kWait;
}

void Task1Window::PartsCreated(QVector<QString> s) {
  text_ += std::format("{} parts added\n", s.size());
  for (int i = old_size_; i < array_size_; ++i) {
    array_[i]._id = new QString(s[i - old_size_]);
  }
  curr_action_ = kNoAction;
}

void Task1Window::PartsCreationFailed() {
  text_ += "Failed to add parts\n";
  PartInputCancelled();
}

void Task1Window::CancelAction() { curr_action_ = kNoAction; }

void Task1Window::ChangeWrapped(int index) {
  part_wrapper_.SetTarget(array_ + index);
}

void Task1Window::SearchSucceed(QJsonObject res) {
  curr_action_ = kNoAction;
  text_ += "Search succeed\n";
  FreeArray();
  array_size_ = res["hits"].toObject()["total"].toObject()["value"].toInt();
  filled_in_ = array_size_;
  if (array_size_ == 0) {
    text_ += "Nothing found\n";
    return;
  }
  array_ = (FactoryPart *)malloc(sizeof(FactoryPart) * array_size_);
  if (array_ == nullptr) {
    QCoreApplication::exit(1);
  }

  QJsonArray vals = res["hits"].toObject()["hits"].toArray();
  for (int i = 0; i < vals.count(); ++i) {
    auto val = vals.at(i).toObject();
    DeserializePart(array_ + i, val);
  }
}
void Task1Window::SearchFailed() {
  curr_action_ = kNoAction;
  text_ += "Search failed!\n";
}

void Task1Window::FreeArray() {
  if (array_ == nullptr) {
    return;
  }
  for (int i = 0; i < filled_in_; ++i) {
    delete array_[i]._id;
    array_[i]._id = nullptr;
  }
  filled_in_ = 0;
  array_size_ = 0;
  free(array_);
}

void Task1Window::IndexDeleted() {
  FreeArray();
  text_ += "Index deleted\n";
  qls_->Send(&index_create_);
}
void Task1Window::IndexDeleteFailed() {
  text_ += "Failed to delete index\n";
  curr_action_ = kNoAction;
}
void Task1Window::IndexCreated() {
  text_ += "Index created\n";
  curr_action_ = kNoAction;
}
void Task1Window::IndexCreateFailed() {
  text_ += "Failed to create index\n";
  curr_action_ = kNoAction;
}

void Task1Window::SendSearch(QJsonObject obj) {
  search_.SetBody(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  qls_->Send(&search_);
}