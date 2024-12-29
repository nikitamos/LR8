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
std::string StringifyConstantCase(const char *c);

/// Abstract input pretty input field
struct Input {
  /** @arg pname name of the Qt property */
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

/// Input that has a buffer that can be trivially accessed
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

/// Window that inputs a meta object based on its properties
class MetaInput : public QObject {
  Q_OBJECT
  Q_PROPERTY(QObject *target READ GetTarget WRITE SetTarget)
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
  /// The same as `SetTarget`, but also displays the values of `new_target`'s
  /// properties
  void PopulateFromTarget(QObject *new_target);
  /// Sets a new target with default window content.
  /// The caller must ensure that `new_target` is valid and properly deleted
  void SetTarget(QObject *new_target);
  QObject *GetTarget();
public slots:
  /// Inserts the target contents into the input fields
  void RepopulateFromTarget();
  /// Populates the properties of the target with the user input
  void Populate();
  void Render();
  /// Resets the inner state of the window after cancellation or submission to
  /// make it visible again. Does NOTHING with the target
  void Reset();
signals:
  /// Emitted when the Submit button is pressed and the target object is
  /// populated
  void Submit(QObject *target);
  /// Emitted when the window is closed or Cancel button is pressed
  void Cancel();

private:
  QObject *t_;
  QList<Input *> item_;
  const char *win_name_;
  bool open_ = true;
  bool cancel_ = false;
};

/// Window that shows the properties of the an array of meta objects and
/// provides `Delete` and `Modify` buttons
class MetaViewer : public QObject {
  Q_OBJECT
public:
  explicit MetaViewer(QObject *provider = nullptr, int size = 0)
      : provider_(provider), size_(size) {}
  /// Get an example object to check the properties
  void SetProvider(QObject *prov) { provider_ = prov; }
  /// Get an index of a current item
  int GetCurrent() const { return curr_; }
  /// Set current index. (The provider must me updated manually)
  void SetCurrent(int newval) { curr_ = newval; }
  /// Set the size of array (for display only)
  void SetCollectionSize(int newsize) { size_ = newsize; }
  int GetCollectionSize() const { return size_; }
  bool IsOpen() const { return open_; }
  void Render();
  /// Closes the window and emits `Closed` signal
  void Close() {
    open_ = false;
    emit Closed();
  }
  /// Does not emit `Closed` signal
  void SetOpen(bool v) { open_ = v; }
signals:
  void Next(int n);
  void Previous(int n);
  void Delete(int n);
  void Modify(int n);
  /// Emitted when the window is closed by user or `Close` method
  void Closed();

private:
  int curr_ = 0;
  int size_ = 1;
  bool open_ = true;
  QObject *provider_;
};

/// The window that allows to select a property of a meta object and its value.
/// Provides buttons `Delete`, `Search` and `Input until`
class FieldValueSelector : public QObject {
  Q_OBJECT
public:
  explicit FieldValueSelector(QMetaType mt);
  virtual ~FieldValueSelector() { ClearInput(); }
  /// Give and example metatype
  void SetPrototype(QMetaType m);

  void Render(const char *name = "Some input");

  QMetaType GetPrototype() { return proto_; }
  /// Return the contents as Elasticsearch Search API query
  QJsonObject JsonBody();

  void SetOpen(bool v) { open_ = v; }
  bool IsOpen() const { return open_; }
  /// Checks that the given object satisfies the condition input by user
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