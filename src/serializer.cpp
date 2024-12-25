#include <QMetaProperty>
#include <qjsonobject.h>

#include "serializer.h"

QJsonObject Serialize(QObject *obj) {
  QJsonObject res;
  for (int i = 1; i < obj->metaObject()->propertyCount(); ++i) {
    QString name = obj->metaObject()->property(i).name();
    auto p = obj->property(name.toStdString().c_str());
    if (p.canConvert<QJsonValue>()) {
      p.convert(QMetaType::QJsonValue);
      res[name] = p.toJsonValue();
    } else {
      res[name] = Serialize(qvariant_cast<QObject *>(p));
    }
  }
  return res;
}