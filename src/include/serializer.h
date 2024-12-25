#pragma once
#include <qjsonobject.h>

QJsonObject Serialize(QObject *obj);
void Deserialize(QObject *target, const QJsonObject &obj);