#pragma once

#include <QObject>
#include <cstdint>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#define PROP(type, x)                                                          \
  Q_PROPERTY(type x READ Get##x WRITE Set##x);                                 \
  inline type Get##x() const { return inner_->x; }                             \
  inline void Set##x(type nval) { inner_->x = nval; }

#define MATERIAL_TAG_DECL {kSteel = 0, kBrass = 1, kNichrome = 2, kTitanium = 3}

enum class MaterialTag : int MATERIAL_TAG_DECL;

typedef struct FactoryPart {
  QString name;
  int32_t count;
  int32_t department_no;
  /// o bloody hell, why?
  union {
    MaterialTag mt;
    int mt_int;
  };
  float weight;
  float volume;
  QString *doc_id{nullptr};
} FactoryPart;

class MetaFactoryPart : public QObject {
  Q_OBJECT
public:
  inline explicit MetaFactoryPart(FactoryPart *inner, QObject *parent = nullptr)
      : QObject(parent), inner_(inner) {}
  inline void SetTarget(FactoryPart *target) { inner_ = target; }
  enum Material : int MATERIAL_TAG_DECL;
  Q_ENUM(Material)

  inline Material Getmaterial() const {
    return static_cast<Material>(inner_->mt_int);
  }
  inline void Setmaterial(Material m) { inner_->mt_int = m; }

  PROP(QString, name)
  Q_PROPERTY(Material material READ Getmaterial WRITE Setmaterial)
  PROP(qint32, count)
  PROP(qint32, department_no)
  PROP(float, weight)
  Q_PROPERTY(float volume READ Getvolume)
  inline float Getvolume() { return inner_->volume; }
  void EvilVolumeCrutch();

private:
  QString name_;
  FactoryPart *inner_;
};

/// Deserializes the given JSON object and intializes the memory pointed by `p`.
void DeserializePart(FactoryPart *p, const QJsonObject &obj);