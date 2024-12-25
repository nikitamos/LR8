#pragma once

#include <QMetaEnum>
#include <cassert>
#include <qmetaobject.h>
#include <qobject.h>

#include <qvariant.h>
#include <string>

using MetaTypeId = QMetaType::Type;
template <typename Tag, MetaTypeId... Types>
class MetaTaggedUnion : public QObject {
  Q_OBJECT
public:
  explicit MetaTaggedUnion(QObject *parent = nullptr)
      : QObject(parent), tag_(0), meta_tag_(QMetaEnum::fromType<Tag>()) {
    assert(meta_tag_.keyCount() == sizeof...(Types));
    AddProperty<Types...>(0);
  }

  const char *GetComboLabel() const { return combo.c_str(); }
  int GetCount() const { return meta_tag_.keyCount(); }

private:
  template <MetaTypeId T, MetaTypeId... Rest> void AddProperty(int index) {
    this->setProperty(meta_tag_.key(index),
                      QVariant::fromMetaType(QMetaType(T), nullptr));
    if constexpr (sizeof...(Rest) != 0) {
      AddProperty<Rest...>(index + 1);
    }
  }
  Tag tag_;
  QMetaEnum meta_tag_;
  std::string combo;
};
