#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

#include "metamagic.h"
#include "qlastic.h"
#include "task1part.h"

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

class Task1MainWindow : public Window {
  Q_OBJECT
public:
  explicit Task1MainWindow(Qlastic *qls, QObject *parent = nullptr)
      : Window(parent), part_wrapper_(&buf_), qls_(qls) {
    meta_input_.SetTarget(&part_wrapper_);
    QObject::connect(&meta_input_, &MetaInput::Submit, this,
                     &Task1MainWindow::PartInputSubmitted);
  }
  virtual ~Task1MainWindow() {}
  virtual void Render();
public slots:
  void PartInputSubmitted(QObject *part);
  // public slots:
  //   void Wait();
private:
  Qlastic *qls_;
  QlBulkCreateDocuments create_{"task1_factory"};
  void DrawMenuWindow();
  FactoryPart buf_;
  Action curr_action_ = kNoAction;
  MetaInput meta_input_{nullptr, "Part input"};
  MetaFactoryPart part_wrapper_;
  bool action_win_open_;
  // bool win_open_ = true;
  FactoryPart *array_ = nullptr;
  int array_size_ = 0;
  int filled_in_ = 0;
  int old_size_ = 0;
  int curr_item_ = 0;
};