#include <cstdlib>
#include <format>

#include <imgui/imgui.h>

#include <qcoreapplication.h>
#include <qjsonarray.h>
#include <qobject.h>

#include "metamagic.h"
#include "qlastic.h"
#include "task1_mainwindow.h"
#include "task1part.h"

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

static float EvilVolumeCrutch(float weight, int index) {
  const float kDensityTable[] = {7850.0, 8700.0, 8400.0, 4540.0};
  return weight / kDensityTable[index];
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
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Whole array") && filled_in_ != 0) {
        meta_viewer_.SetCollectionSize(filled_in_);
        meta_viewer_.SetProvider(&part_wrapper_);
        part_wrapper_.SetTarget(array_);
        meta_viewer_.SetCurrent(0);
        meta_viewer_.SetOpen(true);
        curr_item_ = 0;
        curr_action_ = kViewWhole;
        action_win_open_ = true;
      }
      if (ImGui::MenuItem("Sort")) {
        ShakerSort(array_, filled_in_);
        meta_viewer_.SetCollectionSize(filled_in_);
        meta_viewer_.SetProvider(&part_wrapper_);
        part_wrapper_.SetTarget(array_);
        meta_viewer_.SetCurrent(0);
        meta_viewer_.SetOpen(true);
        curr_item_ = 0;
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

void Task1Window::PartInputSubmitted(QObject *obj) {
  if (next_action_ == kModifyItem) {
    part_wrapper_.EvilVolumeCrutch();
    curr_action_ = kWait;
    next_action_ = kViewWhole;
    update_.SetObject(obj);
    qls_->Send(&update_);

    return;
  }
  if (curr_action_ == kInputUntil) {
    if (!property_selector_.IsSatysfying(&part_wrapper_)) {
      array_ = static_cast<FactoryPart *>(
          realloc(array_, sizeof(FactoryPart) * (++array_size_)));
    }
  }
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
  if (!win_open_) {
    emit Closed();
    return;
  }
  auto io = ImGui::GetIO();

  switch (curr_action_) {
  case kInputTheCount:
    if (action_win_open_) {
      ImGui::SetNextWindowPos(ImVec2(30, 5), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(45, 20), ImGuiCond_FirstUseEver);

      ImGui::Begin("Enter the count", &action_win_open_);
      ImGui::InputInt("", &add_size_);
      if (ImGui::Button("Continue")) {
        if (add_size_ <= 0) {
          curr_action_ = kNoAction;
          text_ += "Input cancelled\n";
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
            curr_action_ = kInputItems;
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
  case kInputUntil:
  case kInputItems:
    meta_input_.Render();
    break;
  case kViewWhole:
    meta_viewer_.Render();
    break;
  case kModifyItem:
    break;
  case kDeleteDocs:
    property_selector_.Render();
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
  QObject::connect(&update_, &QlUpdateDocument::Success, this,
                   &Task1Window::UpdateSucceed);
  QObject::connect(&update_, &QlUpdateDocument::Failure, this,
                   &Task1Window::UpdateFailed);
  QObject::connect(&meta_viewer_, &MetaViewer::Next, this,
                   &Task1Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Previous, this,
                   &Task1Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Closed, this,
                   &Task1Window::CancelAction);
  QObject::connect(&meta_viewer_, &MetaViewer::Delete, this,
                   &Task1Window::DeleteSingleItem);
  QObject::connect(&meta_viewer_, &MetaViewer::Modify, this,
                   &Task1Window::ModifyItem);

  QObject::connect(&search_, &QlSearch::Success, this,
                   &Task1Window::SearchSucceed);
  QObject::connect(&search_, &QlSearch::Failure, this,
                   &Task1Window::SearchFailed);

  QObject::connect(&property_selector_, &FieldValueSelector::Cancel, this,
                   &Task1Window::CancelAction);
  QObject::connect(&property_selector_, &FieldValueSelector::Search, this,
                   &Task1Window::SendSearch);
  QObject::connect(&property_selector_, &FieldValueSelector::Delete, this,
                   &Task1Window::SendSearchDelete);
  QObject::connect(&property_selector_, &FieldValueSelector::InputUntil, this,
                   &Task1Window::InputUntilCondition);

  QObject::connect(&delete_, &QlBulkDeleteDocuments::Success, this,
                   &Task1Window::DeleteSucceed);
  QObject::connect(&delete_, &QlBulkDeleteDocuments::Failure, this,
                   &Task1Window::DeleteFailed);
  search_.SetBody(R"({"fields": ["volume"]})");
  qls_->Send(&search_);
  curr_action_ = kWait;
}

void Task1Window::PartsCreated(QVector<QString> s) {
  text_ += std::format("{} parts added\n", s.size());
  for (int i = old_size_; i < array_size_; ++i) {
    array_[i].doc_id = new QString(s[i - old_size_]);
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
  curr_action_ = next_action_;
  next_action_ = kNoAction;
  text_ += "Search succeed. ";
  FreeArray();
  array_size_ = res["hits"].toObject()["total"].toObject()["value"].toInt();
  filled_in_ = array_size_;
  if (array_size_ == 0) {
    text_ += "Nothing found\n";
    return;
  }
  text_ += std::format("{} found\n", filled_in_);
  array_ =
      static_cast<FactoryPart *>(malloc(sizeof(FactoryPart) * array_size_));
  if (array_ == nullptr) {
    QCoreApplication::exit(1);
  }

  QJsonArray vals = res["hits"].toObject()["hits"].toArray();
  for (int i = 0; i < vals.count(); ++i) {
    auto val = vals.at(i).toObject();
    DeserializePart(array_ + i, val);
  }
  if (curr_action_ == kDeleteDocs) {
    delete_.ClearBody();
    for (int i = 0; i < filled_in_; ++i) {
      delete_.AddDocument(*array_[i].doc_id);
    }
    FreeArray();
    curr_action_ = kWait;
    qls_->Send(&delete_);
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
    delete array_[i].doc_id;
    array_[i].doc_id = nullptr;
    array_[i].name.~QString();
  }
  filled_in_ = 0;
  array_size_ = 0;
  free(array_);
  array_ = nullptr;
}

void Task1Window::SendSearch(QJsonObject obj) {
  curr_action_ = kWait;
  next_action_ = kNoAction;
  QJsonArray fields{"volume"};
  obj["fields"] = fields;
  search_.SetBody(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  qls_->Send(&search_);
}

void Task1Window::SendSearchDelete(QJsonObject obj) {
  curr_action_ = kWait;
  next_action_ = kDeleteDocs;
  search_.SetBody(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  qls_->Send(&search_);
}

void Task1Window::DeleteSucceed() {
  curr_action_ = next_action_;
  next_action_ = kNoAction;
  meta_viewer_.SetCollectionSize(filled_in_);
  text_ += "Deletion succeed. You can select the other documents\n";
}
void Task1Window::DeleteFailed() {
  curr_action_ = kNoAction;
  text_ += "Deletion failed.\n";
}
void Task1Window::DeleteSingleItem(int n) {
  next_action_ = curr_action_;
  curr_action_ = kWait;
  delete_.ClearBody();
  delete_.AddDocument(*array_[n].doc_id);
  delete array_->doc_id;
  for (int i = n + 1; i < filled_in_; ++i) {
    array_[i - 1] = array_[i];
  }
  array_ = static_cast<FactoryPart *>(
      realloc(array_, sizeof(FactoryPart) * (--array_size_)));
  --filled_in_;
  if (n == filled_in_) {
    meta_viewer_.SetCollectionSize(filled_in_ - 1);
    meta_viewer_.SetCurrent(n - 1);
    part_wrapper_.SetTarget(array_ + n - 1);
  }
  qls_->Send(&delete_);
}

void Task1Window::ModifyItem(int n) {
  update_.SetId(*array_[n].doc_id);
  part_wrapper_.SetTarget(array_ + n);
  meta_input_.PopulateFromTarget(&part_wrapper_);
  curr_action_ = kInputItems;
  next_action_ = kModifyItem;
}
void Task1Window::UpdateSucceed() {
  text_ += "Modify succeed\n";
  curr_action_ = next_action_;
  next_action_ = kNoAction;
}
void Task1Window::UpdateFailed() {
  text_ += "Modify failed\n";
  curr_action_ = next_action_;
  next_action_ = kNoAction;
}
void Task1Window::InputUntilCondition() {
  action_win_open_ = true;
  curr_action_ = kInputUntil;
  old_size_ = array_size_;
  array_ = static_cast<FactoryPart *>(
      realloc(array_, sizeof(FactoryPart) * (++array_size_)));
  ++curr_item_;
  meta_input_.Reset();
}