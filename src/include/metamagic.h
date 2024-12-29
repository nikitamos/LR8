#pragma once

#include <cctype>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <qcontainerfwd.h>
#include <qjsonobject.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <random>
#include <string>

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

std::string StringifySnakeCase(const char *text);

struct Input {
  explicit Input(QString pname);
  virtual ~Input() {}
  virtual void Render() = 0;
  virtual QVariant Get() = 0;
  virtual void Set(QVariant val) = 0;
  QString property_name;
  std::string text;
  std::string GetLabel() { return "##" + std::to_string(rand()); }
  std::string label;
  static std::mt19937_64 rand;
  static Input *FromType(QMetaType meta, QString property_name);
};

template <typename T> struct BufInput : public Input {
  T buf{};
  explicit BufInput(QString name) : Input(name) {}
  virtual QVariant Get() override { return buf; }
  virtual void Set(QVariant val) override { buf = qvariant_cast<T>(val); }
};

struct StdStringInput : public Input {
  explicit StdStringInput(QString p) : Input(p) {}
  virtual void Render() override {
    ImGui::Text("%s", text.c_str());
    ImGui::InputText(label.c_str(), &buf);
  }
  virtual QVariant Get() override { return QString::fromStdString(buf); }
  virtual void Set(QVariant val) override {
    buf = val.toString().toStdString();
  }
  std::string buf;
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
  void PopulateFromTarget(QObject *new_target);
  void SetTarget(QObject *new_target);
  QObject *GetTarget();
public slots:
  void Populate();
  void Render();
  void Reset();
signals:
  /// The signal is emitted when the Submit button is pressed after the target
  /// object is populated
  void Submit(QObject *target);
  void Cancel();

private:
  QObject *t_;
  QList<Input *> item_;
  const char *win_name_;
  bool open_ = true;
  bool cancel_ = false;
};

class MetaViewer : public QObject {
  Q_OBJECT
public:
  explicit MetaViewer(QObject *provider = nullptr, int size = 0)
      : provider_(provider), size_(size) {}
  void SetProvider(QObject *prov) { provider_ = prov; }
  int GetCurrent() const { return curr_; }
  void SetCurrent(int newval) { curr_ = newval; }
  void SetCollectionSize(int newsize) { size_ = newsize; }
  int GetCollectionSize() const { return size_; }
  bool IsOpen() const { return open_; }
  void Render();
  void Close() {
    open_ = false;
    emit Closed();
  }
  void SetOpen(bool v) { open_ = v; }
signals:
  /// Emitted when Next button is pressed. Argument is the new index
  void Next(int n);
  /// Emitted wher Prev button is pressed. Argument is the new number
  void Previous(int n);
  void Delete(int n);
  void Modify(int n);
  void Closed();

private:
  int curr_ = 0;
  int size_ = 1;
  bool open_ = true;
  QObject *provider_;
};

class FieldValueSelector : public QObject {
  Q_OBJECT
public:
  explicit FieldValueSelector(QMetaType mt);
  virtual ~FieldValueSelector() { ClearInput(); }
  void SetPrototype(QMetaType m);

  void Render(const char *name = "Some input");

  QMetaType GetPrototype() { return proto_; }
  QJsonObject JsonBody();

  void SetOpen(bool v) { open_ = v; }
  bool IsOpen() const { return open_; }
  bool IsSatysfying(QObject *check);
signals:
  void Delete(QJsonObject);
  void Search(QJsonObject);
  void InputUntil(QJsonObject);
  void Cancel();

private:
  bool all_ = false;
  bool open_ = true;
  void ClearInput();
  int selected_ = 0;
  QVector<Input *> inputs_;
  QMetaType proto_;
  std::string label_;
};