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
  kInputItems,
  kInputUntil,
  kViewWhole,
  kModifyItem,
  kDeleteDocs,
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
  void PartInputSubmitted(QObject *obj);
  void PartInputCancelled();

  void PartsCreated(QVector<QString> ids);
  void PartsCreationFailed();

  void ChangeWrapped(int index);
  void CancelAction();

  void SearchSucceed(QJsonObject res);
  void SearchFailed();

  void SendSearch(QJsonObject obj);
  void SendSearchDelete(QJsonObject obj);

  void DeleteSingleItem(int n);
  void DeleteSucceed();
  void DeleteFailed();

  void ModifyItem(int n);

  void UpdateSucceed();
  void UpdateFailed();

  void InputUntilCondition();

private:
  void FreeArray();
  Qlastic *qls_;
  QlBulkCreateDocuments create_{"task1_factory"};
  QlBulkDeleteDocuments delete_{"task1_factory"};
  QlSearch search_{"task1_factory", QJsonDocument()};
  QlUpdateDocument update_{"task1_factory"};
  FieldValueSelector property_selector_;

  std::string text_{"\"Was mich nicht umbringt, macht mich staerker.\"\n"};
  void DrawMenuWindow();
  Action curr_action_ = kNoAction;
  Action next_action_ = kNoAction;
  MetaInput meta_input_{nullptr, "Part input"};
  MetaViewer meta_viewer_;
  MetaFactoryPart part_wrapper_;

  bool action_win_open_;
  FactoryPart *array_ = nullptr;
  int array_size_ = 0;
  int filled_in_ = 0;
  int old_size_ = 0;
  int curr_item_ = 0;
  int add_size_ = 2;
};