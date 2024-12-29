#pragma once
#include <qjsonobject.h>
#include <qobject.h>

/// Recursively serializes the properties of given object
QJsonObject Serialize(QObject *obj);