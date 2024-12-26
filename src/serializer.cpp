#include <QMetaProperty>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlogging.h>
#include <qobjectdefs.h>
#include <qvariant.h>

#include "serializer.h"
#include "task1part.h"

QJsonObject Serialize(QObject *obj) {
  QJsonObject res;
  const auto *meta = obj->metaObject();
  for (int i = 1; i < meta->propertyCount(); ++i) {
    if (!meta->property(i).isWritable()) {
      continue;
    }
    QString name = meta->property(i).name();
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

void DeserializePart(FactoryPart *p, const QJsonObject &obj) {
  QJsonObject src = obj["_source"].toObject();
  QString tmpstr = obj["_id"].toString();
  p->doc_id = new QString(tmpstr);
  p->count = src["count"].toInt();
  p->department_no = src["department_no"].toInt();
  p->mt_int = src["material"].toInt();
  p->weight = static_cast<float>(src["weight"].toDouble());
  p->volume = static_cast<float>(obj["fields"]["volume"][0].toDouble());
  new (&p->name) QString(src["name"].toString());
}