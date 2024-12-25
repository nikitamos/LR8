#pragma once

#include <QObject>
#include <cstdint>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>

#define PROP(type, x)                                                          \
  Q_PROPERTY(type x READ Get##x WRITE Set##x);                                 \
  type Get##x() const { return inner_->x; }                                    \
  void Set##x(type nval) { inner_->x = nval; }

#define MATERIAL_TAG_DECL {kSteel = 0, kBrass = 1, kNichrome = 2, kTitan = 3}

enum MaterialTag MATERIAL_TAG_DECL;

typedef struct FactoryPart {
  QString name;
  int32_t count;
  int32_t department_no;
  MaterialTag mt;
  float weight;
  float volume;
  QString *_id{nullptr};
} FactoryPart;

class MetaFactoryPart : public QObject {
  Q_OBJECT
public:
  explicit MetaFactoryPart(FactoryPart *inner, QObject *parent = nullptr)
      : QObject(parent), inner_(inner) {}
  void SetTarget(FactoryPart *target) { inner_ = target; }
  enum Material MATERIAL_TAG_DECL;
  Q_ENUM(Material)

  Material Getmaterial() const { return (Material)inner_->mt; }
  void Setmaterial(Material m) { inner_->mt = (MaterialTag)m; }

  PROP(QString, name)
  Q_PROPERTY(Material material READ Getmaterial WRITE Setmaterial)
  PROP(qint32, count)
  PROP(qint32, department_no)
  PROP(qint32, weight)
  PROP(float, volume)
private:
  QString name_;
  FactoryPart *inner_;
};

void DeserializePart(FactoryPart *p, const QJsonObject &obj);