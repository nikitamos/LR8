#pragma once

#include <cctype>
#include <imgui/imgui.h>
#include <qcontainerfwd.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <random>
#include <string>

#include "metaenum.h"
#include "window.h"

inline QMetaEnum MetaTypeToMetaEnum(const QMetaType &t) {
  assert(t.flags() & QMetaType::IsEnumeration);
  // Returns the metaobject enclosing the enum (read the docs~)
  const auto *enclosing = t.metaObject();
  std::string_view sv(t.name());
  sv = sv.substr(sv.rfind(':') + 1);
  int index = enclosing->indexOfEnumerator(sv.begin());
  assert(index >= 0);
  return enclosing->enumerator(index);
}

struct Input {
  explicit Input(QString pname);
  virtual ~Input() {}
  virtual void Render() = 0;
  virtual QVariant Get() = 0;
  QString property_name;
  std::string text;
  std::string GetLabel() { return "##" + std::to_string(rand()); }
  std::string label;
  static std::mt19937_64 rand;
  static Input *FromType(QMetaType meta, QString property_name);
};

template <typename T> struct BufInput : public Input {
  T buf;
  explicit BufInput(QString name) : Input(name) {}
  virtual QVariant Get() override { return buf; }
};

inline std::mt19937_64 Input::rand{};

struct IntInput : public BufInput<int> {
  explicit IntInput(QString p) : BufInput(p) {}
  virtual void Render() override {
    ImGui::Text("%s", text.c_str());
    ImGui::InputInt(label.c_str(), &buf);
  }
};

struct FloatInput : public BufInput<float> {
  explicit FloatInput(QString p) : BufInput(p) {}
  virtual void Render() override {
    ImGui::Text("%s", text.c_str());
    ImGui::InputFloat(label.c_str(), &buf);
  }
};

struct TextInput : public Input {
  explicit TextInput(QString p) : Input(p) { memset(buf, 0, sizeof(buf)); }
  virtual void Render() override {
    ImGui::Text("%s", text.c_str());
    ImGui::InputText(label.c_str(), buf, sizeof(buf));
  }
  virtual QVariant Get() override { return QString::fromUtf8(buf); }
  char buf[80];
};

struct EnumInput : public BufInput<int> {
  explicit EnumInput(QString p, QMetaEnum meta);
  virtual void Render() override;
  std::string s;
  QMetaEnum meta;
  QVector<std::string> item_names;
};

class MetaInput : public QObject {
  Q_OBJECT
  Q_PROPERTY(QObject *target READ GetTarget WRITE SetTarget NOTIFY Submit)
public:
  explicit MetaInput(QObject *target = nullptr, const char *name = nullptr,
                     QObject *parent = nullptr)
      : t_(target), win_name_(name), QObject(parent) {
    if (target != nullptr) {
      SetTarget(target);
    }
  }
  virtual ~MetaInput() {
    for (auto &it : item_) {
      delete it;
    }
  }
  void SetTarget(QObject *new_target);
  QObject *GetTarget();
public slots:
  void Populate();
  void Render();
signals:
  /// The signal is emitted when the Submit button is pressed after the target
  /// object is populated
  void Submit(QObject *target);

private:
  QObject *t_;
  QList<Input *> item_;
  const char *win_name_;
  bool open_ = true;
};
/*
class MetaViewer : Window {
  Q_OBJECT
public:
signals:
};
*/