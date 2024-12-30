#pragma once

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qurl.h>
#include <qurlquery.h>

#include "serializer.h"

/// An abstract class representing an Elasticsearch API call
class QlasticOperation : public QObject {
  Q_OBJECT
public:
  QlasticOperation() {};
  virtual ~QlasticOperation() { repl_ = nullptr; }
  /// Sends the request via given manager and the base url
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) = 0;
public slots:
  virtual void RequestFinished() = 0;

protected:
  void SetupReply(QNetworkReply *reply);
  QNetworkReply *repl_ = nullptr;
};

class QlCreateIndex : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlCreateIndex(QString index) : index_(index) {}
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;
signals:
  void Success();
  void Failure(QNetworkReply::NetworkError);

private:
  QString index_;
};

class QlDeleteIndex : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlDeleteIndex(QString index) : index_(index) {}
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;
signals:
  void Success();
  void Failure(QNetworkReply::NetworkError);

private:
  QString index_;
};

class QlSearch : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlSearch(QString index, QJsonDocument body);
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;
  inline void SetBody(QString body) { body_ = body; }
  inline void SetParams(QString params) { params_ = params; }
  inline void SetParams(QUrlQuery q) { params_ = q.toString(); }
signals:
  void Failure();
  void Success(QJsonObject res);

private:
  QString params_;
  QString body_;
  QString index_;
};

class QlBulkCreateDocuments : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlBulkCreateDocuments(QString index) : index_(index) {}
  /// Serializes the `doc` and adds it to the request
  inline void AddDocument(QObject *doc) {
    docs_.push_back(QJsonDocument(Serialize(doc)));
  }
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;

signals:
  void Success(QVector<QString> ids);
  void Failure(QNetworkReply::NetworkError err, int code);

private:
  QString index_;
  QVector<QJsonDocument> docs_;
};

class QlBulkDeleteDocuments : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlBulkDeleteDocuments(QString index) : index_(index) {}
  /// Add document with given `id` to the request
  inline void AddDocument(QString id) { ids_.push_back(id); }
  inline void ClearBody() { ids_.clear(); }
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;

signals:
  void Success();
  void Failure();

private:
  QString index_;
  QVector<QString> ids_;
};

class QlUpdateDocument : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlUpdateDocument(QString index, QString id = "")
      : index_(index), id_(id) {}
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override;
  /// Set the id of the document
  void SetId(QString id) { id_ = id; }
  /// Serializes the object and uses its properties to form a request
  QlUpdateDocument &SetObject(QObject *obj);
  inline QString GetId() const { return id_; }

signals:
  void Success();
  void Failure();

private:
  QString index_;
  QString id_;
  QString body_;
};

class Qlastic : public QObject {
  Q_OBJECT
public:
  explicit Qlastic(QUrl serv, QObject *parent = nullptr);
  virtual ~Qlastic() {};

  inline void Send(QlasticOperation *op) { op->SendVia(mgr_, url_); }

private:
  QUrl url_;
  QNetworkAccessManager mgr_;
};