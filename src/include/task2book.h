#pragma once

#include <QObject>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>

#define PROP(type, x)                                                          \
  Q_PROPERTY(type x READ Get##x WRITE Set##x);                                 \
  type Get##x() const { return inner_->x; }                                    \
  void Set##x(type nval) { inner_->x = nval; }

#define DEPOSITORY_DECL                                                        \
  {kMain = 0, kSubsidiary = 1, kReadingHall = 2, kRareEditions = 3}

enum class DepositoryTag : int DEPOSITORY_DECL;

typedef struct LibraryBook {
  QString registry_number;
  QString title;
  QString author;
  int publishing_year;
  QString publishing_house;
  int page_count;
  QString *doc_id{nullptr};
  union {
    DepositoryTag depository;
    int depository_int;
  };
} LibraryBook;

class MetaLibraryBook : public QObject {
  Q_OBJECT
public:
  enum Depository DEPOSITORY_DECL;
  Q_ENUM(Depository)
  explicit MetaLibraryBook(LibraryBook *inner, QObject *parent = nullptr)
      : QObject(parent), inner_(inner) {}
  void SetTarget(LibraryBook *target) { inner_ = target; }

  PROP(QString, registry_number)
  Q_PROPERTY(Depository depository READ Getdepository WRITE Setdepository)
  PROP(QString, title)
  PROP(QString, author)
  PROP(int, publishing_year)
  PROP(QString, publishing_house)
  PROP(int, page_count)

  Depository Getdepository() const {
    return (Depository)inner_->depository_int;
  }
  void Setdepository(Depository d) { inner_->depository_int = d; }

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
  PROP(int, publishing_year)

private:
  LibraryBook *inner_;
};

void DeserializeBook(LibraryBook *p, const QJsonObject &obj);