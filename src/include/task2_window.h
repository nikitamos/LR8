#pragma once

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "metamagic.h"
#include "qlastic.h"
#include "task2book.h"
#include "window.h"

enum Action {
  kInputTheCount,
  kInputItems,
  kInputUntil,
  kViewWhole,
  kModifyItem,
  kDeleteDocs,
  kWait,
  kInputProperty,
  kNoAction
};

class Task2Window : public Window {
  Q_OBJECT
public:
  explicit Task2Window(Qlastic *qls, QObject *parent = nullptr);
  virtual ~Task2Window() { FreeArray(); }
  virtual void Render();
public slots:
  void PartInputSubmitted(QObject *obj);
  void PartInputCancelled();

  void PartsCreated(QVector<QString> ids);
  void PartsCreationFailed();

  void ChangeWrapped(int index);
  void CancelAction();

  void SearchSucceed(QJsonObject res);
  void SearchFailed();

  void SendSearch(QJsonObject obj);
  void SendSearchWithSort(QObject * /* unused */);

  void DeleteSingleItem(int n);
  void DeleteSucceed();
  void DeleteFailed();

  void ModifyItem(int n);

  void UpdateSucceed();
  void UpdateFailed();

private:
  void FreeArray();
  void DrawMenuWindow();
  Qlastic *qls_;
  QlBulkCreateDocuments create_{"task2_library"};
  QlBulkDeleteDocuments delete_{"task2_library"};
  QlSearch search_{"task2_library", QJsonDocument()};
  QlUpdateDocument update_{"task2_library"};
  MetaInput property_selector_{nullptr, "Input search data"};

  std::string text_;
  LibraryBook buf_;
  MetaBookSelector search_selector_;
  Action curr_action_ = kNoAction;
  Action next_action_ = kNoAction;
  MetaInput meta_input_{nullptr, "Book input"};
  MetaViewer meta_viewer_;
  MetaLibraryBook part_wrapper_;
  MetaBookSelector book_selector_wrapper_;
  LibraryBook selector_buf_;

  bool action_win_open_;
  LibraryBook *array_ = nullptr;
  int array_size_ = 0;
  int filled_in_ = 0;
  int old_size_ = 0;
  int curr_item_ = 0;
  int add_size_ = 2;
};