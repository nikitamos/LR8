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
  char name[80];
  int32_t count;
  int32_t department_no;
  MaterialTag mt;
  union {
    int32_t steel;
    float brass;
  } material;
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
  Q_PROPERTY(QString name READ Getname WRITE Setname)
  QString Getname() const { return name_; }
  void Setname(QString nv) { name_ = nv; }

  Q_PROPERTY(Material material READ Getmaterial WRITE Setmaterial)
  Material Getmaterial() const { return (Material)inner_->mt; }
  void Setmaterial(Material m) { inner_->mt = (MaterialTag)m; }

  PROP(qint32, count)
  PROP(qint32, department_no)
  PROP(qint32, weight)
  PROP(float, volume)
  //   PROP(Material, material)
private:
  QString name_;
  FactoryPart *inner_;
};