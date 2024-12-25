#pragma once

#include <QObject>
#include <qobject.h>

class Window : public QObject {
  Q_OBJECT
public:
  Q_PROPERTY(bool open READ IsOpen WRITE SetOpen)
  explicit Window(QObject *parent) : QObject(parent) {}
  virtual ~Window() {}
  inline void SetOpen(bool nval) { win_open_ = nval; }
  inline bool IsOpen() const { return win_open_; }
  virtual void Render() = 0;
signals:
  void Closed();

protected:
  bool win_open_;
};