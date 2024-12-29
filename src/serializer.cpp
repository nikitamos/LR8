#include <QMetaProperty>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qvariant.h>

#include "serializer.h"
#include "task1part.h"
#include "task2book.h"

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

void DeserializeBook(LibraryBook *p, const QJsonObject &obj) {
  QJsonObject src = obj["_source"].toObject();
  QString tmpstr = obj["_id"].toString();
  p->doc_id = new QString(tmpstr);
  new (&p->author) QString(src["author"].toString());
  new (&p->publishing_house) QString(src["publishing_house"].toString());
  new (&p->registry_number) QString(src["registry_number"].toString());
  new (&p->title) QString(src["title"].toString());
  p->page_count = src["page_count"].toInt();
  p->publishing_year = src["publishing_year"].toInt();
  p->depository_int = src["depository"].toInt();
}