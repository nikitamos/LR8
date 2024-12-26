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

#define MATERIAL_TAG_DECL {kSteel = 0, kBrass = 1, kNichrome = 2, kTitanium = 3}

typedef struct LibraryBook {
  QString registry_number;
  QString title;
  QString author;
  int publishing_year;
  QString publishing_house;
  int page_count;
  QString *doc_id{nullptr};
} LibraryBook;

class MetaLibraryBook : public QObject {
  Q_OBJECT
public:
  explicit MetaLibraryBook(LibraryBook *inner, QObject *parent = nullptr)
      : QObject(parent), inner_(inner) {}
  void SetTarget(LibraryBook *target) { inner_ = target; }

  PROP(QString, registry_number)
  PROP(QString, title)
  PROP(QString, author)
  PROP(int, publishing_year)
  PROP(QString, publishing_house)
  PROP(int, page_count)

private:
  QString name_;
  LibraryBook *inner_;
};

class MetaBookSelector : public QObject {
  Q_OBJECT
public:
  explicit MetaBookSelector(LibraryBook *inner, QObject *parent = nullptr)
      : QObject(parent), inner_(inner) {}
  void SetTarget(LibraryBook *target) { inner_ = target; }

  PROP(QString, author)
  PROP(int, publishing_year)

private:
  QString name_;
  LibraryBook *inner_;
};

void DeserializeBook(LibraryBook *p, const QJsonObject &obj);