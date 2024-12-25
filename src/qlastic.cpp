#include "qlastic.h"
#include <qabstractsocket.h>
#include <qhttp2configuration.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qurl.h>

Qlastic::Qlastic(QUrl serv, QObject *parent)
    : url_(serv), mgr_(this), QObject(parent) {
  // QObject::connect(&mgr_, &QNetworkAccessManager::finished, this,
  //                  &Qlastic::RequestFinished);
  QObject::connect(this, &Qlastic::Send, this, &Qlastic::SendHelper);
}

void QlCreateIndex::SendVia(QNetworkAccessManager &mgr, QUrl base_url) {
  QUrl req_url = base_url;
  req_url.setPath("/" + index_, QUrl::StrictMode);

  QNetworkRequest req(req_url);
  repl_ = mgr.put(req, "");
  QObject::connect(repl_, &QNetworkReply::finished, this,
                   &QlCreateIndex::RequestFinished);
  qDebug() << "Exit create index";
}

void QlDeleteIndex::SendVia(QNetworkAccessManager &mgr, QUrl base_url) {
  base_url.setPath("/" + index_);
  QNetworkRequest req(base_url);
  SetupReply(mgr.deleteResource(req));
  qDebug() << "Exit DeleteIndex";
}

void QlSearch::SendVia(QNetworkAccessManager &mgr, QUrl base_url) {
  base_url.setPath("/" + index_ + "/_search");
  base_url.setQuery(params_);
  QNetworkRequest req(base_url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  qDebug() << base_url;
  SetupReply(mgr.get(req, body_.toUtf8()));
  qDebug() << "Exit search!";
}

void QlBulkCreateDocuments::SendVia(QNetworkAccessManager &mgr, QUrl base_url) {
  base_url.setPath("/" + index_ + "/_bulk");
  QNetworkRequest req(base_url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QString body;
  for (auto &doc : docs_) {
    body += R"({"create" : {}})";
    body += '\n';
    body += doc.toJson(QJsonDocument::Compact) + '\n';
  }
  SetupReply(mgr.post(req, body.toUtf8()));
  docs_.clear();
}

void QlBulkCreateDocuments::RequestFinished() {
  if (repl_->error() != 0) {
    emit Failure(
        repl_->error(),
        repl_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
  } else {
    QJsonObject items = QJsonDocument::fromJson(repl_->readAll()).object();
    if (items["errors"].toBool()) {
      emit Failure(QNetworkReply::ProtocolFailure, -1);
    } else {
      QVector<QString> ids;
      QJsonArray vals = items["items"].toArray();
      for (auto v : vals) {
        ids.push_back(v.toObject()["create"].toObject()["_id"].toString());
      }
      qDebug() << "new ids: " << ids;
      emit Success(ids);
    }
  }
  repl_->deleteLater();
  repl_ = nullptr;
}

void QlBulkDeleteDocuments::SendVia(QNetworkAccessManager &mgr, QUrl base_url) {
  base_url.setPath("/" + index_ + "/_bulk");
  QNetworkRequest req(base_url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QString body;
  for (auto &id : ids_) {
    // clang-format off
    body += QJsonDocument(QJsonObject{{
      "delete", QJsonObject{{
        "_id", id
      }}
    }}).toJson(QJsonDocument::Compact) + "\n";
    // clang-format on
  }
  std::cerr << body.toStdString() << '\n';
  SetupReply(mgr.post(req, body.toUtf8()));
  ids_.clear();
}

void QlBulkDeleteDocuments::RequestFinished() {
  if (repl_->error() != 0) {
    emit Failure();
  } else {
    QJsonObject items = QJsonDocument::fromJson(repl_->readAll()).object();
    if (items["errors"].toBool()) {
      emit Failure();
    } else {
      emit Success();
    }
  }
  repl_->deleteLater();
  repl_ = nullptr;
}