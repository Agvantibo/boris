/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/***************************************************************************
 *            item.cpp
 *
 *  Mon Nov 11 12:54:00 CEST 2019
 *  Copyright 2019 Lars Muldjord
 *  muldjordlars@gmail.com
 ****************************************************************************/

/*
 *  This file is part of Boris.
 *
 *  Boris is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Boris is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Boris; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#include "item.h"
#include "scripthandler.h"
#include "soundmixer.h"
#include "settings.h"

#include "SFML/Audio.hpp"

#include <stdio.h>
#include <math.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QDate>
#include <QTime>

extern QList<Behaviour> itemList;
extern SoundMixer soundMixer;

Item::Item(const int &x, const int &y, const int &size, const QString &item, Settings *settings)
{
  this->settings = settings;
  
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::ToolTip);
  setFrameShape(QFrame::NoFrame);
  setStyleSheet("background:transparent");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  
  for(int a = 0; a < itemList.count(); ++a) {
    if(itemList.at(a).file == item) {
      curItem = a;
      break;
    }
  }

  setScene(new QGraphicsScene);
  scene()->setSceneRect(0.0, 0.0, 32, 32 + 1); // + 1 to make room for shadow

  origShadow.load(":shadow.png");
  shadowSprite = this->scene()->addPixmap(origShadow);
  shadowSprite->setOpacity(0.35);

  itemSprite = this->scene()->addPixmap(QPixmap());
  itemSprite->setPos(0, 0);

  scriptSprite = this->scene()->addPixmap(QPixmap());
  scriptSprite->setPos(0, 0); // To make room for shadow

  setCursor(QCursor(QPixmap(":mouse_closet.png"), 15, 16));

  setFixedSize(size, size + (size / 32)); // To make room for shadow
  scale(size / 32.0, size / 32.0);
  move(x, y);
  if(settings->itemTimeout > 0) {
    QTimer::singleShot(settings->itemTimeout * 1000, this, &Item::destroy);
  }

  if(itemList.at(curItem).allowFlip && qrand() %2) {
    flipFrames = true;
  } else {
    flipFrames = false;
  }

  // Reset all script variables
  scriptVars.clear();

  // Clear script image canvas
  scriptImage.fill(Qt::transparent);
  drawing = false;

  animTimer.setInterval(0);
  animTimer.setSingleShot(true);
  connect(&animTimer, &QTimer::timeout, this, &Item::nextFrame);
  animTimer.start();

  show();
}

Item::~Item()
{
}

QPixmap Item::getShadow(const QPixmap &sprite)
{
  QImage image = sprite.toImage();
  int firstLeft = sprite.width();
  int bottom = 0;
  for(int row = 0; row < sprite.height(); ++row) {
    QRgb *rowBits = (QRgb *)image.constScanLine(row);
    for(int col = 0; col < sprite.width(); ++col) {
      if(qAlpha(rowBits[col]) != 0) {
        if(bottom < row) {
          bottom = row;
        }
        if(firstLeft > col) {
          firstLeft = col;
          break;
        }
      }
    }
  }
  shadowSprite->setPos(firstLeft, bottom - 30);
  int firstRight = 0;
  for(int row = 0; row < sprite.height(); ++row) {
    QRgb *rowBits = (QRgb *)image.constScanLine(row);
    for(int col = sprite.width() - 1; col >= 0; --col) {
      if(qAlpha(rowBits[col]) != 0 && firstRight < col) {
        firstRight = col;
        break;
      }
    }
  }
  if(firstRight == 0) {
    firstRight = sprite.width();
  }
  if(firstLeft == sprite.width()) {
    firstLeft = 0;
  }
  int shadowWidth = firstRight - firstLeft;
  QImage shadow = origShadow.toImage().scaled(shadowWidth, origShadow.height());
  return QPixmap::fromImage(shadow);
}

void Item::runScript(int &stop)
{
  // Update current stat variables for scripting use
  QPoint p = QCursor::pos();
  scriptVars["mousex"] = p.x();
  scriptVars["mousey"] = p.y();
  scriptVars["mdist"] = getDistance(QCursor::pos());
  scriptVars["msec"] = getSector(QCursor::pos());
  QDate date = QDate::currentDate();
  QTime time = QTime::currentTime();
  scriptVars["day"] = date.day();
  scriptVars["month"] = date.month();
  scriptVars["year"] = date.year();
  scriptVars["hour"] = time.hour();
  scriptVars["minute"] = time.minute();
  scriptVars["second"] = time.second();

  if(!drawing) {
    scriptImage.fill(Qt::transparent);
  }

  ScriptHandler scriptHandler(&scriptImage, &drawing, settings, itemList.at(curItem).labels, itemList.at(curItem).defines, scriptVars, pos(), size);
  connect(&scriptHandler, &ScriptHandler::setCurFrame, this, &Item::setCurFrame);
  scriptHandler.runScript(stop, itemList.at(curItem).frames.at(curFrame).script);

  scriptSprite->setPixmap(QPixmap::fromImage(scriptImage));
}

void Item::nextFrame()
{
  sanityCheck();
  
  if(curFrame >= itemList.at(curItem).frames.count()) {
    curFrame = 0;
  }

  QTime frameTimer;
  frameTimer.start();

  if(flipFrames) {
    QImage flipped = itemList.at(curItem).frames.at(curFrame).sprite.toImage().mirrored(true, false);
    itemSprite->setPixmap(QPixmap::fromImage(flipped));
    shadowSprite->setPixmap(getShadow(QPixmap::fromImage(flipped)));
  } else {
    itemSprite->setPixmap(itemList.at(curItem).frames.at(curFrame).sprite);
    shadowSprite->setPixmap(getShadow(itemList.at(curItem).frames.at(curFrame).sprite));
  }

  if(settings->sound && itemList.at(curItem).frames.at(curFrame).soundBuffer != nullptr) {
    if(itemList.at(curItem).pitchLock) {
      soundMixer.playSound(itemList.at(curItem).frames.at(curFrame).soundBuffer,
                           (float)pos().x() / (float)settings->desktopWidth * 2.0 - 1.0, 1.0);
    } else {
      soundMixer.playSound(itemList.at(curItem).frames.at(curFrame).soundBuffer,
                           (float)pos().x() / (float)settings->desktopWidth * 2.0 - 1.0,
                           0.95 + (qrand() % 100) / 1000.0);
    }
  }

  if(itemList.at(curItem).frames.at(curFrame).dx != 0 ||
     itemList.at(curItem).frames.at(curFrame).dy != 0) {
    moveItem(itemList.at(curItem).frames.at(curFrame).dx,
              itemList.at(curItem).frames.at(curFrame).dy,
              flipFrames);
  }

  int frameTime = itemList.at(curItem).frames.at(curFrame).time;
  int elapsedTime = frameTimer.elapsed();
  if(elapsedTime < frameTime) {
    frameTime -= elapsedTime;
  }
  if(frameTime <= 5) {
    frameTime = 5;
  }
  animTimer.setInterval(frameTime);

  int stop = 0; // Will be > 0 if a goto, behav or break command is run
  runScript(stop);
  if(stop == 1) {
    // In case of 'goto' curFrame has been set in scriptHandler
  } else if(stop == 2) {
    // In case of 'break' it will destroy the item
    destroy();
    return;
  } else if(stop == 3) {
    // In case of 'stop' it will cease any frame and animation progression
    animTimer.stop();
    return;
  } else {
    curFrame++;
  }

  animTimer.start();
}

void Item::moveItem(int dX, int dY, const bool &flipped)
{
  sanityCheck();
  
  int maxX = QApplication::desktop()->width() - size;
  int maxY = QApplication::desktop()->height() - size;

  if(dX == 666) {
    dX = qrand() % maxX;
  } else {
    if(flipped) {
      dX *= -1;
    }
    dX = pos().x() + (dX * ceil(size / 32.0));
  }
  if(dY == 666) {
    dY = qrand() % maxY;
  } else {
    dY = pos().y() + (dY * ceil(size / 32.0));
  }
  
  move(dX, dY);
}

void Item::sanityCheck()
{
  int minX = - size;
  int maxX = QApplication::desktop()->width();
  int minY = 0 - (size / 2);
  int maxY = QApplication::desktop()->height() - height();

  // Make sure Item is not located outside boundaries
  if(pos().y() < minY) {
    move(pos().x(), minY);
  }
  if(pos().y() > maxY) {
    move(pos().x(), maxY);
  }
  if(pos().x() > maxX) {
    move(minX, pos().y());
  }
  if(pos().x() < minX) {
    move(maxX, pos().y());
  }
}

int Item::getDistance(const QPoint &p)
{
  int xA = p.x();
  int yA = p.y();
  int xB = pos().x() + (size / 2);
  int yB = pos().y() + (size / 2);

  return sqrt((yB - yA) * (yB - yA) + (xB - xA) * (xB - xA));
}

int Item::getSector(const QPoint &p)
{
  // Center coordinate of Item
  QPoint b(pos().x() + (size / 2), pos().y() + (size / 2));

  // First find seg coordinate on x
  int xScale = abs(b.y() - p.y()) * 2;
  double xScaleSeg = xScale / 3.0;
  double xZero = b.x() - (xScale / 2.0);
  int xSeg = -1;
  if(p.x() < xZero + xScaleSeg) {
    xSeg = 0;
  } else if(p.x() < xZero + (xScaleSeg * 2)) {
    xSeg = 1;
  } else if(p.x() >= xZero + (xScaleSeg * 2)) {
    xSeg = 2;
  }
  // Then find seg coordinate on y
  int yScale = abs(b.x() - p.x()) * 2;
  double yScaleSeg = yScale / 3.0;
  double yZero = b.y() - (yScale / 2.0);
  int ySeg = -1;
  if(p.y() < yZero + yScaleSeg) {
    ySeg = 0;
  } else if(p.y() < yZero + (yScaleSeg * 2)) {
    ySeg = 1;
  } else if(p.y() >= yZero + (yScaleSeg * 2)) {
    ySeg = 2;
  }
  int pointSector = -1;
  if(xSeg == 0) {
    if(ySeg == 0) {
      pointSector = Direction::NorthWest;
    } else if(ySeg == 1) {
      pointSector = Direction::West;
    } else if(ySeg == 2) {
      pointSector = Direction::SouthWest;
    }
  } else if(xSeg == 1) {
    if(ySeg == 0) {
      pointSector = Direction::North;
    } else if(ySeg == 1) {
      pointSector = Direction::None;
    } else if(ySeg == 2) {
      pointSector = Direction::South;
    }
  } else if(xSeg == 2) {
    if(ySeg == 0) {
      pointSector = Direction::NorthEast;
    } else if(ySeg == 1) {
      pointSector = Direction::East;
    } else if(ySeg == 2) {
      pointSector = Direction::SouthEast;
    }
  }
  return pointSector;
}

void Item::mousePressEvent(QMouseEvent* event)
{
  if(event->button() == Qt::LeftButton) {
    destroy();
  }
}

void Item::setCurFrame(const int &frame)
{
  curFrame = frame;
}

void Item::destroy()
{
  delete this;
}
