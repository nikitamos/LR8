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
  kDeleteDocs,
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
  void CancelAction();

  void SearchSucceed(QJsonObject res);
  void SearchFailed();

  void IndexDeleted();
  void IndexDeleteFailed();

  void IndexCreated();
  void IndexCreateFailed();

  void SendSearch(QJsonObject obj);
  void SendSearchDelete(QJsonObject obj);

  void DeleteSucceed();
  void DeleteFailed();

private:
  void FreeArray();
  Qlastic *qls_;
  QlBulkCreateDocuments create_{"task1_factory"};
  QlBulkDeleteDocuments delete_{"task1_factory"};
  QlSearch search_{"task1_factory", QJsonDocument()};
  QlDeleteIndex index_delete_{"task1_factory"};
  QlCreateIndex index_create_{"task1_factory"};
  FieldValueSelector property_selector_;

  std::string text_{"Was mich nicht umbringt, macht mich staerker.\n"};
  void DrawMenuWindow();
  FactoryPart buf_;
  Action curr_action_ = kNoAction;
  Action next_action_ = kNoAction;
  MetaInput meta_input_{nullptr, "Part input"};
  MetaViewer meta_viewer_;
  MetaFactoryPart part_wrapper_;
  [[deprecated]]
  bool action_win_open_;
  FactoryPart *array_ = nullptr;
  int array_size_ = 0;
  int filled_in_ = 0;
  int old_size_ = 0;
  int curr_item_ = 0;
  int add_size_ = 2;
};