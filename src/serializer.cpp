#include <QMetaProperty>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlogging.h>
#include <qobjectdefs.h>
#include <qvariant.h>

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

void Deserialize(QObject *target, const QJsonObject &obj) {
  const auto *meta = target->metaObject();
  for (int i = 0; i < meta->propertyCount(); ++i) {
    auto property = meta->property(i);
    QString property_name = property.name();
    if (obj.contains(property_name)) {
      QVariant val = obj[property_name].toVariant();
      if (val.metaType() == property.metaType()) {
        target->setProperty(property_name.toUtf8(), val);
      } else {
        qDebug() << "Can not convert some shit!" << property.metaType().name();
      }
    }
  }
}