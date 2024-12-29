#include <cstdlib>
#include <format>

#include <imgui/imgui.h>
#include <qcoreapplication.h>
#include <qjsonarray.h>
#include <qobject.h>

#include "metamagic.h"
#include "qlastic.h"
#include "task2_window.h"
#include "task2book.h"

static void FreeLibraryBookFields(LibraryBook *b) {
  delete b->doc_id;
  b->doc_id = nullptr;
  b->~LibraryBook();
}

void Task2Window::DrawMenuWindow() {
  auto io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 6, io.DisplaySize.y - 3),
                           ImGuiCond_Always);

  ImGui::Begin("Database of a library", &win_open_, ImGuiWindowFlags_MenuBar);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Input")) {
      if (ImGui::MenuItem("The count")) {
        action_win_open_ = true;
        curr_action_ = kInputTheCount;
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
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Select books")) {
      if (ImGui::MenuItem("All")) {
        search_.SetBody("{}");
        curr_action_ = kWait;
        next_action_ = kNoAction;
        qls_->Send(&search_);
      }
      if (ImGui::MenuItem("Published after")) {
        curr_action_ = kInputProperty;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  ImGui::Text("%s", text_.c_str());
  ImGui::End();
}

void Task2Window::PartInputSubmitted(QObject *obj) {
  if (next_action_ == kModifyItem) {
    curr_action_ = kWait;
    next_action_ = kViewWhole;
    update_.SetObject(obj);
    qls_->Send(&update_);

    return;
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
    meta_input_.RepopulateFromTarget();
  }
}

void Task2Window::PartInputCancelled() {
  array_size_ = old_size_;
  filled_in_ = old_size_;
  array_ = static_cast<LibraryBook *>(
      realloc(array_, array_size_ * sizeof(LibraryBook)));
  curr_action_ = kNoAction;
}

void Task2Window::Render() {
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
          LibraryBook *new_arr = static_cast<LibraryBook *>(
              realloc(array_, sizeof(LibraryBook) * (array_size_ + add_size_)));
          if (new_arr == nullptr) {
            ImGui::Text("Unable to reallocate memory ");
          } else {
            memset(new_arr + array_size_, 0, sizeof(LibraryBook) * add_size_);
            old_size_ = array_size_;
            array_size_ += add_size_;
            array_ = new_arr;
            curr_action_ = kInputItems;
            curr_item_ = old_size_;
            meta_input_.Reset();
            part_wrapper_.SetTarget(array_ + old_size_);
            meta_input_.RepopulateFromTarget();
          }
        }
      }
      ImGui::End();
    } else {
      curr_action_ = kNoAction;
    }
    break;
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

Task2Window::Task2Window(Qlastic *qls, QObject *parent)
    : Window(parent), part_wrapper_(&buf_), qls_(qls), search_selector_(&buf_),
      book_selector_wrapper_(&selector_buf_) {
  property_selector_.SetTarget(&book_selector_wrapper_);
  meta_input_.SetTarget(&part_wrapper_);
  QObject::connect(&meta_input_, &MetaInput::Submit, this,
                   &Task2Window::PartInputSubmitted);
  QObject::connect(&meta_input_, &MetaInput::Cancel, this,
                   &Task2Window::PartInputCancelled);
  QObject::connect(&create_, &QlBulkCreateDocuments::Success, this,
                   &Task2Window::PartsCreated);
  QObject::connect(&create_, &QlBulkCreateDocuments::Failure, this,
                   &Task2Window::PartsCreationFailed);
  QObject::connect(&update_, &QlUpdateDocument::Success, this,
                   &Task2Window::UpdateSucceed);
  QObject::connect(&update_, &QlUpdateDocument::Failure, this,
                   &Task2Window::UpdateFailed);
  QObject::connect(&meta_viewer_, &MetaViewer::Next, this,
                   &Task2Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Previous, this,
                   &Task2Window::ChangeWrapped);
  QObject::connect(&meta_viewer_, &MetaViewer::Closed, this,
                   &Task2Window::CancelAction);
  QObject::connect(&meta_viewer_, &MetaViewer::Delete, this,
                   &Task2Window::DeleteSingleItem);
  QObject::connect(&meta_viewer_, &MetaViewer::Modify, this,
                   &Task2Window::ModifyItem);

  QObject::connect(&search_, &QlSearch::Success, this,
                   &Task2Window::SearchSucceed);
  QObject::connect(&search_, &QlSearch::Failure, this,
                   &Task2Window::SearchFailed);

  QObject::connect(&property_selector_, &MetaInput::Cancel, this,
                   &Task2Window::CancelAction);
  QObject::connect(&property_selector_, &MetaInput::Submit, this,
                   &Task2Window::SendSearchWithSort);

  QObject::connect(&delete_, &QlBulkDeleteDocuments::Success, this,
                   &Task2Window::DeleteSucceed);
  QObject::connect(&delete_, &QlBulkDeleteDocuments::Failure, this,
                   &Task2Window::DeleteFailed);
  search_.SetBody(R"({"fields": ["volume"]})");
  qls_->Send(&search_);
  curr_action_ = kWait;
}

void Task2Window::PartsCreated(QVector<QString> s) {
  text_ += std::format("{} parts added\n", s.size());
  for (int i = old_size_; i < array_size_; ++i) {
    array_[i].doc_id = new QString(s[i - old_size_]);
  }
  curr_action_ = kNoAction;
}

void Task2Window::PartsCreationFailed() {
  text_ += "Failed to add parts\n";
  PartInputCancelled();
}

void Task2Window::CancelAction() { curr_action_ = kNoAction; }

void Task2Window::ChangeWrapped(int index) {
  part_wrapper_.SetTarget(array_ + index);
}

void Task2Window::SearchSucceed(QJsonObject res) {
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
      static_cast<LibraryBook *>(malloc(sizeof(LibraryBook) * array_size_));
  if (array_ == nullptr) {
    QCoreApplication::exit(1);
  }

  QJsonArray vals = res["hits"].toObject()["hits"].toArray();
  for (int i = 0; i < vals.count(); ++i) {
    auto val = vals.at(i).toObject();
    DeserializeBook(array_ + i, val);
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
void Task2Window::SearchFailed() {
  curr_action_ = kNoAction;
  text_ += "Search failed!\n";
}

void Task2Window::FreeArray() {
  if (array_ == nullptr) {
    return;
  }
  for (int i = 0; i < filled_in_; ++i) {
    FreeLibraryBookFields(array_ + i);
  }
  filled_in_ = 0;
  array_size_ = 0;
  free(array_);
  array_ = nullptr;
}

void Task2Window::SendSearch(QJsonObject obj) {
  curr_action_ = kWait;
  next_action_ = kNoAction;
  QJsonArray fields{"volume"};
  obj["fields"] = fields;
  search_.SetBody(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  qls_->Send(&search_);
}

void Task2Window::SendSearchWithSort(QObject * /* unused */) {
  curr_action_ = kWait;
  next_action_ = kNoAction;

  // clang-format off
  QJsonObject res2{
    {
      "query", QJsonObject{{
            {"range", QJsonObject{{
                "publishing_year", QJsonObject{{"gte", selector_buf_.publishing_year}}
            }}}}
        }},
    {
        "sort", QJsonArray{
            QJsonObject{{"author", "asc"}}
        }
    }
  };
  // clang-format on

  search_.SetBody(QJsonDocument(res2).toJson(QJsonDocument::Compact));
  qls_->Send(&search_);
}

void Task2Window::DeleteSucceed() {
  curr_action_ = next_action_;
  next_action_ = kNoAction;
  meta_viewer_.SetCollectionSize(filled_in_);
  text_ += "Deletion succeed. You can select the other documents\n";
}
void Task2Window::DeleteFailed() {
  curr_action_ = kNoAction;
  text_ += "Deletion failed.\n";
}
void Task2Window::DeleteSingleItem(int n) {
  next_action_ = curr_action_;
  curr_action_ = kWait;
  delete_.ClearBody();
  delete_.AddDocument(*array_[n].doc_id);
  delete array_[n].doc_id;
  for (int i = n + 1; i < filled_in_; ++i) {
    array_[i - 1] = array_[i];
  }
  array_[filled_in_ - 1].doc_id = nullptr;
  FreeLibraryBookFields(array_ + filled_in_ - 1);
  array_ = static_cast<LibraryBook *>(
      realloc(array_, sizeof(LibraryBook) * (--array_size_)));
  --filled_in_;
  if (n == filled_in_) {
    meta_viewer_.SetCollectionSize(filled_in_ - 1);
    meta_viewer_.SetCurrent(n - 1);
    part_wrapper_.SetTarget(array_ + n - 1);
  }
  if (filled_in_ == 0) {
    next_action_ = kNoAction;
  }
  qls_->Send(&delete_);
}

void Task2Window::ModifyItem(int n) {
  update_.SetId(*array_[n].doc_id);
  part_wrapper_.SetTarget(array_ + n);
  meta_input_.PopulateFromTarget(&part_wrapper_);
  curr_action_ = kInputItems;
  next_action_ = kModifyItem;
}
void Task2Window::UpdateSucceed() {
  text_ += "Modify succeed\n";
  curr_action_ = next_action_;
  next_action_ = kNoAction;
}
void Task2Window::UpdateFailed() {
  text_ += "Modify failed\n";
  curr_action_ = next_action_;
  next_action_ = kNoAction;
}