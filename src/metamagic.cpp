#include "metamagic.h"
#include "imgui/imgui.h"
#include <qcborcommon.h>
#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qlogging.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qvariant.h>

QMetaEnum MetaTypeToMetaEnum(const QMetaType &t) {
  assert(t.flags() & QMetaType::IsEnumeration);
  // Returns the metaobject enclosing the enum (read the docs~)
  const auto *enclosing = t.metaObject();
  std::string_view sv(t.name());
  sv = sv.substr(sv.rfind(':') + 1);
  int index = enclosing->indexOfEnumerator(sv.begin());
  assert(index >= 0);
  return enclosing->enumerator(index);
}

std::string StringifyConstantCase(const char *c) {
  std::string s(c);
  s.erase(0, 1);
  for (int i = 1; i < s.size(); ++i) {
    if (isupper(s[i]) != 0) {
      s[i] = tolower(s[i]);
      s.insert(i, " ");
    }
  }
  return s;
}

Input::Input(QString pname) : property_name(pname) {
  auto text2 = property_name;
  text2.replace("_", " ");
  text = text2.toStdString();
  text[0] = toupper(text[0]);
  label = GetLabel();
}

Input *Input::FromType(QMetaType meta, QString property_name) {
  Input *res = nullptr;
  switch (meta.id()) {
  case QMetaType::Float:
    res = new FloatInput(property_name);
    break;
  case QMetaType::Int:
    res = new IntInput(property_name);
    break;
  case QMetaType::QString:
    res = new StdStringInput(property_name);
    break;
  default:
    if ((meta.flags() & QMetaType::IsEnumeration) != 0) {
      res = new EnumInput(property_name, MetaTypeToMetaEnum(meta));
    } else {
      qDebug() << "Warning: ignoring unknown metatype of property:"
               << meta.name();
    }
  }
  return res;
}

EnumInput::EnumInput(QString p, QMetaEnum meta) : BufInput(p), meta(meta) {
  for (int i = 0; i < meta.keyCount(); ++i) {
    item_names.push_back(StringifyConstantCase(meta.valueToKey(i)));
  }
}

void EnumInput::Render() {
  ImGui::Text("%s", this->text.c_str());
  if (ImGui::BeginCombo(this->label.c_str(), item_names[buf].c_str())) {
    for (int i = 0; i < meta.keyCount(); ++i) {
      bool selected;
      if (ImGui::Selectable(item_names[i].c_str())) {
        buf = i;
      }
      if (i == buf) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

void StdStringInput::Render() {
  ImGui::Text("%s", text.c_str());
  ImGui::InputText(label.c_str(), &buf);
}
QVariant StdStringInput::Get() { return QString::fromStdString(buf); }
void StdStringInput::Set(QVariant val) { buf = val.toString().toStdString(); }

void IntInput::Render() {
  ImGui::Text("%s", text.c_str());
  ImGui::InputInt(label.c_str(), &buf);
}

void FloatInput::Render() {
  ImGui::Text("%s", text.c_str());
  ImGui::InputFloat(label.c_str(), &buf);
}

MetaInput::~MetaInput() {
  for (auto &it : item_) {
    delete it;
  }
}

void MetaInput::Populate() {
  qDebug() << "pop" << t_;
  for (auto &i : item_) {
    t_->setProperty(i->property_name.toUtf8(), i->Get());
  }
}

void MetaInput::SetTarget(QObject *new_target) {
  const auto *meta = new_target->metaObject();
  for (auto &i : item_) {
    delete i;
  }
  t_ = new_target;
  item_.clear();

  for (int i = 1; i < meta->propertyCount(); ++i) {
    if (!meta->property(i).isWritable()) {
      continue;
    }
    const char *name = meta->property(i).name();
    auto property = new_target->property(name);
    Input *r = Input::FromType(property.metaType(), name);
    if (r != nullptr) {
      item_.push_back(r);
    }
  }
}

void MetaInput::PopulateFromTarget(QObject *new_target) {
  SetTarget(new_target);
  RepopulateFromTarget();
}

void MetaInput::Render() {
  if (cancel_) {
    return;
  }
  ImGui::Begin(win_name_, &open_);
  for (auto &i : item_) {
    i->Render();
  }
  if (ImGui::Button("Submit")) {
    Populate();
    emit Submit(t_);
  } else if (!open_ && !cancel_) {
    cancel_ = true;
    emit Cancel();
  }
  ImGui::End();
}

void MetaInput::Reset() {
  cancel_ = false;
  open_ = true;
}

QObject *MetaInput::GetTarget() { return t_; }

void MetaInput::RepopulateFromTarget() {
  for (auto &i : this->item_) {
    i->Set(t_->property(i->property_name.toUtf8()));
  }
}

void MetaViewer::Render() {
  if (!open_) {
    emit Closed();
    return;
  }
  if (ImGui::Begin("Item viewer", &open_)) {
    std::string s = std::format("{} of {}", curr_ + 1, size_);
    ImGui::Text("%s", s.c_str());
    ImGui::NewLine();

    const auto *meta = provider_->metaObject();
    for (int i = 1; i < meta->propertyCount(); ++i) {
      auto prop = meta->property(i);
      if (prop.isReadable()) {
        std::string item = StringifySnakeCase(prop.name()) + ": ";
        QVariant val = prop.read(provider_);
        if (val.typeId() == QMetaType::Float ||
            val.typeId() == QMetaType::Double) {
          item += std::format(
              "{}", round(prop.read(provider_).toDouble() * 1000.0) / 1000.0);
        } else if ((val.metaType().flags() & QMetaType::IsEnumeration) != 0) {
          item +=
              StringifyConstantCase(prop.read(provider_).toString().toUtf8());
        } else {
          item += prop.read(provider_).toString().toStdString();
        }
        ImGui::Text("%s", item.c_str());
      }
    }
    if (curr_ > 0) {
      if (ImGui::Button("Previous")) {
        --curr_;
        emit Previous(curr_);
      }
      ImGui::SameLine();
    }
    if (curr_ < size_ - 1) {
      if (ImGui::Button("Next")) {
        ++curr_;
        emit Next(curr_);
      }
    } else {
      if (ImGui::Button("Finish")) {
        Close();
      }
    }
    ImGui::NewLine();
    if (ImGui::Button("Delete")) {
      emit Delete(curr_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Modify")) {
      emit Modify(curr_);
    }
    ImGui::End();
  }
}

FieldValueSelector::FieldValueSelector(QMetaType mt) : selected_(0) {
  selected_ = 0;
  label_ = "##" + std::to_string(Input::rand());
  SetPrototype(mt);
}
void FieldValueSelector::SetPrototype(QMetaType m) {
  const auto *mobj = m.metaObject();
  proto_ = m;
  ClearInput();
  inputs_.clear();

  for (int i = 1; i < mobj->propertyCount(); ++i) {
    std::string normalized = StringifySnakeCase(mobj->property(i).name());
    Input *inp =
        Input::FromType(mobj->property(i).metaType(), mobj->property(i).name());
    if (inp != nullptr) {
      inputs_.push_back(inp);
    }
  }
}

void FieldValueSelector::Render(const char *name) {
  if (!open_) {
    emit Cancel();
  }
  ImGui::Begin(name, &open_);
  if (ImGui::Checkbox("Select All", &all_) || all_) {
    ImGui::NewLine();
  } else {
    if (ImGui::NewLine(),
        ImGui::BeginCombo(label_.c_str(), inputs_[selected_]->text.c_str())) {
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
    if (ImGui::Button("Input until")) {
      emit InputUntil(JsonBody());
      open_ = false;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete")) {
    emit Delete(JsonBody());
    open_ = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Search")) {
    emit Search(JsonBody());
    open_ = false;
  }
  ImGui::End();
}

QJsonObject FieldValueSelector::JsonBody() {
  if (all_) {
    return QJsonObject();
  }

  auto qjsval = inputs_[selected_]->Get().toJsonValue();
  // clang-format off
  QJsonObject res2{{
    "query", QJsonObject{{
      "match", QJsonObject{{
          inputs_[selected_]->property_name, QJsonObject{{
            "query", qjsval
          }}
        }}
      }}
  }};
  // clang-format on
  return res2;
}

void FieldValueSelector::ClearInput() {
  for (auto &i : inputs_) {
    delete i;
  }
}

bool FieldValueSelector::IsSatysfying(QObject *check) {
  auto prop = inputs_[selected_]->property_name;
  auto check_value = check->property(prop.toUtf8());
  if (!check_value.isValid()) {
    return false;
  }
  QVariant input = inputs_[selected_]->Get();

  if (check_value.metaType() != input.metaType()) {
    if ((check_value.metaType().flags() & QMetaType::IsEnumeration) != 0 &&
        input.canConvert(QMetaType::Int)) {
      return check_value.toInt() == input.toInt();
    }
    return false;
  }
  return input == check_value;
}

std::string StringifySnakeCase(const char *text) {
  QString text2 = text;
  text2.replace("_", " ");
  std::string res = text2.toStdString();
  res[0] = toupper(res[0]);
  return res;
}
