#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

#include "metamagic.h"
#include "qlastic.h"
#include "task1part.h"
#include "window.h"

enum Action {
  kInputTheCount,
  kInputItemsCount,
  kInputUntil,
  kViewWhole,
  kViewSearch,
  kModifyItem,
  kModifyRemove,
  kModifyRemoveAll,
  kWait,
  kNoAction
};

class Task1Window : public Window {
  Q_OBJECT
public:
  explicit Task1Window(Qlastic *qls, QObject *parent = nullptr);
  virtual ~Task1Window() {}
  virtual void Render();
public slots:
  void PartInputSubmitted(QObject *part);
  void PartInputCancelled();

  void PartsCreated(QVector<QString> ids);
  void PartsCreationFailed();

  void ChangeWrapped(int index);
  void ViewerClosed();

private:
  Qlastic *qls_;
  std::string text_;
  QlBulkCreateDocuments create_{"task1_factory"};
  void DrawMenuWindow();
  FactoryPart buf_;
  Action curr_action_ = kNoAction;
  MetaInput meta_input_{nullptr, "Part input"};
  MetaViewer meta_viewer_;
  MetaFactoryPart part_wrapper_;
  bool action_win_open_;
  // bool win_open_ = true;
  FactoryPart *array_ = nullptr;
  int array_size_ = 0;
  int filled_in_ = 0;
  int old_size_ = 0;
  int curr_item_ = 0;
  int add_size_ = 2;
};