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

static std::string StringifySnakeCase(const char *text) {
  QString text2 = text;
  text2.replace("_", " ");
  std::string res = text2.toStdString();
  res[0] = toupper(res[0]);
  return res;
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
  T buf{};
  explicit BufInput(QString name) : Input(name) {}
  virtual QVariant Get() override { return buf; }
};

struct StdStringInput : public Input {
  explicit StdStringInput(QString p) : Input(p) {}
  virtual void Render() override {
    ImGui::Text("%s", text.c_str());
    ImGui::InputText(label.c_str(), &buf);
  }
  virtual QVariant Get() override { return QString::fromStdString(buf); }
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
signals:
  /// Emitted when Next button is pressed. Argument is the new index
  void Next(int n);
  /// Emitted wher Prev button is pressed. Argument is the new number
  void Previous(int n);
  void Custom(int n);
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
  explicit FieldValueSelector(QMetaType mt) : selected_(0) {
    label_ = "##" + std::to_string(Input::rand());
    SetPrototype(mt);
  }
  virtual ~FieldValueSelector() { ClearInput(); }
  void SetPrototype(QMetaType m) {
    const auto *mobj = m.metaObject();
    proto_ = m;
    ClearInput();
    inputs_.clear();

    for (int i = 1; i < mobj->propertyCount(); ++i) {
      std::string normalized = StringifySnakeCase(mobj->property(i).name());
      Input *inp = Input::FromType(mobj->property(i).metaType(),
                                   mobj->property(i).name());
      if (inp != nullptr) {
        inputs_.push_back(inp);
      }
    }
  }

  void Render(const char *name = "Some input") {
    ImGui::Begin(name);
    if (ImGui::BeginCombo(label_.c_str(), inputs_[selected_]->text.c_str())) {
      for (int i = 0; i < inputs_.size(); ++i) {
        bool selected;
        if (ImGui::Selectable(inputs_[i]->text.c_str())) {
          selected_ = i;
        }
        if (i == selected_) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    inputs_[selected_]->Render();
    ImGui::NewLine();
    if (ImGui::Button("Modify")) {
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) {
    }
    ImGui::SameLine();
    if (ImGui::Button("Search")) {
    }
    ImGui::End();
  }

  QMetaType GetPrototype() { return proto_; }
  QJsonObject JsonPair() {
    QJsonObject res;
    res[inputs_[selected_]->property_name] =
        inputs_[selected_]->Get().toJsonValue();
    return res;
  }

private:
  void ClearInput() {
    for (auto &i : inputs_) {
      delete i;
    }
  }
  int selected_ = 0;
  QVector<Input *> inputs_;
  QMetaType proto_;
  std::string label_;
};