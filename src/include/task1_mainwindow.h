#pragma once

#include <qjsondocument.h>
#include <qjsonobject.h>
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
  kInputProperty,
  kNoAction
};

class Task1Window : public Window {
  Q_OBJECT
public:
  explicit Task1Window(Qlastic *qls, QObject *parent = nullptr);
  virtual ~Task1Window() { FreeArray(); }
  virtual void Render();
public slots:
  void PartInputSubmitted(QObject *part);
  void PartInputCancelled();

  void PartsCreated(QVector<QString> ids);
  void PartsCreationFailed();

  void ChangeWrapped(int index);
  void ViewerClosed();

  void SearchSucceed(QJsonObject res);
  void SearchFailed();

  void IndexDeleted();
  void IndexDeleteFailed();

  void IndexCreated();
  void IndexCreateFailed();

private:
  void FreeArray();
  Qlastic *qls_;
  QlBulkCreateDocuments create_{"task1_factory"};
  QlSearch search_{"task1_factory", QJsonDocument()};
  QlDeleteIndex index_delete_{"task1_factory"};
  QlCreateIndex index_create_{"task1_factory"};
  FieldValueSelector field_selector_;

  std::string text_{"Was mich nicht umbringt, macht mich staerker.\n"};
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