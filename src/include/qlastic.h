#pragma once

#include "serializer.h"
#include <QNetworkAccessManager>
#include <functional>
#include <iostream>
#include <qjsondocument.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qurl.h>

class QlasticOperation : public QObject {
  Q_OBJECT
public:
  QlasticOperation() {};
  virtual ~QlasticOperation() {
    // deleteLater?
    delete repl_;
  }
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) = 0;
public slots:
  virtual void RequestFinished() = 0;

protected:
  void SetupReply(QNetworkReply *reply) {
    repl_ = reply;
    QObject::connect(repl_, &QNetworkReply::finished, this,
                     &QlasticOperation::RequestFinished);
  }
  QNetworkReply *repl_ = nullptr;
};

class QlCreateIndex : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlCreateIndex(QString index) : index_(index) {}
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override {
    qDebug() << "CreateIndex req. finish!\n";
    if (repl_->error() != 0) {
      emit Failure(repl_->error());
    } else {
      emit Success();
    }
  }
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
  virtual void RequestFinished() override {
    qDebug() << "CreateIndex req. finish!\n";
    if (repl_->error() != 0) {
      emit Failure(repl_->error());
    } else {
      emit Success();
    }
  }
signals:
  void Success();
  void Failure(QNetworkReply::NetworkError);

private:
  QString index_;
};

class QlSearch : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlSearch(QString index, QJsonDocument *body)
      : index_(index), body_(body->toJson()) {}
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override {
    std::cerr << "Error: " << repl_->error() << '\n';
    qDebug() << "Search finished" << repl_->readAll();
  }

private:
  QString params_;
  QString body_;
  QString index_;
};

class QlBulkCreateDocuments : public QlasticOperation {
  Q_OBJECT
public:
  explicit QlBulkCreateDocuments(QString index) : index_(index) {}
  void AddDocument(QObject *doc) {
    auto tmp = QJsonDocument(Serialize(doc));
    qDebug() << tmp.toJson(); // TODO: remove tmp
    docs_.push_back(tmp);
  }
  virtual void SendVia(QNetworkAccessManager &mgr, QUrl base_url) override;
  virtual void RequestFinished() override {
    std::cerr << "Error: " << repl_->error() << '\n';
    qDebug() << "Create finished" << repl_->readAll();
  }

private:
  QString index_;
  QVector<QJsonDocument> docs_;
};

class Qlastic : public QObject {
  Q_OBJECT
public:
  explicit Qlastic(QUrl serv, QObject *parent = nullptr);
  virtual ~Qlastic() {};
  bool IsPendingRequest() const { return is_pending_; }

signals:
  void Send(QlasticOperation *op);

public slots:
  void RequestFinished() { is_pending_ = false; }

protected slots:
  void SendHelper(QlasticOperation *op) {
    is_pending_ = true;
    op->SendVia(mgr_, url_);
  }

private:
  bool is_pending_ = false;
  QUrl url_;
  QNetworkAccessManager mgr_;
};