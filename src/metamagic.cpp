#include "metamagic.h"
#include <qlogging.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qvariant.h>

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
    res = new TextInput(property_name);
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
    std::string s(meta.valueToKey(i));
    s.erase(0, 1);
    for (int i = 1; i < s.size(); ++i) {
      if (isupper(s[i]) != 0) {
        s[i] = tolower(s[i]);
        s.insert(i, " ");
      }
    }
    item_names.push_back(s);
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

void MetaInput::Populate() {
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
    const char *name = meta->property(i).name();
    auto property = new_target->property(name);
    Input *r = Input::FromType(property.metaType(), name);
    if (r != nullptr) {
      item_.push_back(r);
    }
  }
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

void MetaViewer::Render() {
  const auto *meta = provider_->metaObject();
  for (int i = 1; i < meta->propertyCount(); ++i) {
    auto prop = meta->property(i);
    if (prop.isReadable()) {
      std::string item = StringifySnakeCase(prop.name()) + ": ";
      item += prop.read(provider_).toString().toStdString();
      ImGui::Text("%s", item.c_str());
      ImGui::NewLine();
    }
  }
  if (curr_ > 0) {
    if (ImGui::Button("Previous")) {
      --curr_;
      emit Previous(curr_);
    }
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
}